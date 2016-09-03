/*
 * Copyright (c) 2010 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include <syscall.h>
#include <lostio.h>
#include <page.h>
#include <env.h>

enum ppb_entry_type {
    PPB_CMDLINE_ARG = 1,
    PPB_ENV_VAR,
    PPB_STREAM,
};

struct ppb_entry {
    enum ppb_entry_type type;
    ptrdiff_t           value;
    size_t              len;
};

struct proc_param_block {
    size_t              num;
    struct ppb_entry    entries[];
};

struct ppb_builder {
    struct proc_param_block*    ppb;
    size_t                      ppb_dir_size;
    size_t                      ppb_dir_used;
    size_t                      ppb_data_used;
};

extern pid_t __init_exec(const char* cmd, int ppb_shm_id);

static struct ppb_builder* ppb_builder_init(void)
{
    struct ppb_builder* b;

    b = malloc(sizeof(*b));
    if (b == NULL) {
        return NULL;
    }

    b->ppb              = mem_allocate(PAGE_SIZE, 0);
    b->ppb_dir_size     = PAGE_SIZE;
    b->ppb_dir_used     = sizeof(struct proc_param_block);
    b->ppb_data_used    = 0;

    memset(b->ppb, 0, PAGE_SIZE);

    return b;
}

static void* pgrealloc(void* old, size_t old_size, size_t new_size)
{
    uint8_t* new;

    new = mem_allocate(new_size, 0);
    if (new == NULL) {
        return NULL;
    }

    memset(new + old_size, 0, new_size - old_size);
    memcpy(new, old, old_size);
    mem_free(old, old_size);

    return new;
}

static int ppb_add_entry(struct ppb_builder* b, enum ppb_entry_type type,
    const void* value, size_t len)
{
    size_t used;
    int index;
    void* new;

    /* Verzeichnis vergroessern falls noetig */
    used = b->ppb_dir_used + sizeof(struct ppb_entry);

    if (used > b->ppb_dir_size) {
        new = pgrealloc(b->ppb, b->ppb_dir_size, NUM_PAGES(used));
        if (new == NULL) {
            return -ENOMEM;
        }

        b->ppb = new;
        b->ppb_dir_size = NUM_PAGES(b->ppb_dir_used);
    }

    /* Daten eintragen */
    index = b->ppb->num++;
    b->ppb->entries[index].type  = type;
    b->ppb->entries[index].value = (ptrdiff_t) value;
    b->ppb->entries[index].len   = len;

    b->ppb_dir_used += sizeof(struct ppb_entry);
    b->ppb_data_used += len;

    return 0;
}

static int ppb_builder_finish(struct ppb_builder* b)
{
    int shm;
    size_t size;
    uint8_t* p;
    int i, num_entries;
    ptrdiff_t offset;
    struct ppb_entry* entry;

    /* Shared Memory anlegen */
    size = b->ppb_dir_used + b->ppb_data_used;

    shm = create_shared_memory(size);
    if (shm < 0) {
        return shm;
    }

    p = open_shared_memory(shm);
    if (p == NULL) {
        return -ENOMEM;
    }

    /* Daten kopieren*/
    num_entries = b->ppb->num;
    offset = b->ppb_dir_used;

    for (i = 0; i < num_entries; i++) {
        entry = &b->ppb->entries[i];
        memcpy(p + offset, (void*) entry->value, entry->len);
        if (entry->type == PPB_ENV_VAR) {
            free((void*) entry->value);
        }
        entry->value = offset;
        offset += entry->len;
    }

    memcpy(p, b->ppb, b->ppb_dir_used);

    /* Strukturen freigeben */
    mem_free(b->ppb, NUM_PAGES(b->ppb_dir_used));
    free(b);

    return shm;
}

static int ppb_add_cloned_stdio(struct ppb_builder* b,
                                lio_stream_t *remote, int num)
{
    FILE* stdio[] = { stdin, stdout, stderr };
    int i;

    assert(num <= 3);
    for (i = 0; i < num; i++) {
        lio_stream_t orig, copy;
        int ret;

        orig = file_get_stream(stdio[i]);
        if (orig < 0) {
            continue;
        }

        copy = lio_dup(orig, -1);
        if (copy < 0) {
            return copy;
        }

        remote[i] = lio_pass_fd(copy, 1);
        if (remote[i] < 0) {
            lio_close(copy);
            return remote[i];
        }

        ret = ppb_add_entry(b, PPB_STREAM, &remote[i], sizeof(remote[i]));
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}

static int ppb_add_cloned_env(struct ppb_builder* b)
{
    int i, cnt;
    const char* name;
    const char* value;
    size_t name_len, value_len;
    char *buf;
    int ret;

    /* FIXME Evtl. wäre es geschickt, envvar_lock zu halten */
    p();
    cnt = getenv_count();
    for (i = 0; i < cnt; i++) {
        name = getenv_name_by_index(i);
        if (name == NULL) {
            ret = -EFAULT;
            goto out;
        }
        name_len = strlen(name);

        value = getenv_index(i);
        if (value == NULL) {
            ret = -EFAULT;
            goto out;
        }
        value_len = strlen(value);

        buf = malloc(name_len + value_len + 2);
        if (buf == NULL) {
            ret = -ENOMEM;
            goto out;
        }
        memcpy(buf, name, name_len + 1);
        memcpy(buf + name_len + 1, value, value_len + 1);

        ppb_add_entry(b, PPB_ENV_VAR, buf, name_len + value_len + 2);
    }

    ret = 0;
out:
    v();
    return ret;
}

int ppb_from_argv(const char* const argv[])
{
    struct ppb_builder* b;
    int shm;
    const char* const* p;
    int ret;

    /* Muss hier deklariert werden statt in ppb_add_cloned_stdio(), sonst ist
     * der Pointer nicht mehr gültig, wenn er in ppb_builder_finish() benutzt
     * wird. */
    lio_stream_t remote_streams[3];

    b = ppb_builder_init();
    if (b == NULL) {
        errno = ENOMEM;
        return -1;
    }

    p = argv;
    while (*p != NULL) {
        ppb_add_entry(b, PPB_CMDLINE_ARG, *p, strlen(*p) + 1);
        p++;
    }

    ret = ppb_add_cloned_env(b);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    ret = ppb_add_cloned_stdio(b, remote_streams, 3);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    shm = ppb_builder_finish(b);
    if (shm < 0) {
        errno = -shm;
        return -1;
    }

    return shm;
}

pid_t init_execv(const char* file, const char* const argv[])
{
    int shm;
    pid_t ret;

    shm = ppb_from_argv(argv);
    if (shm == -1) {
        return -1;
    }

    ret = __init_exec(file, shm);
    close_shared_memory(shm);

    return ret;
}

bool ppb_is_valid(struct proc_param_block* ppb, size_t ppb_size)
{
    size_t offset;
    size_t len;
    size_t remaining;
    int i;

    len = sizeof(*ppb) + ppb->num * sizeof(*ppb->entries);
    if (ppb_size < len) {
        return false;
    }

    remaining = ppb_size - len;
    for (i = 0; i < ppb->num; i++) {
        len = ppb->entries[i].len;
        if (remaining < len) {
            return false;
        }

        offset = ppb->entries[i].value;
        if (offset > ppb_size || offset + len > ppb_size) {
            return false;
        }

        remaining -= len;
    }

    return true;
}

int ppb_get_argc(struct proc_param_block* ppb, size_t ppb_size)
{
    int i;
    int argc = 0;

    if (!ppb_is_valid(ppb, ppb_size)) {
        return -EINVAL;
    }

    for (i = 0; i < ppb->num; i++) {
        if (ppb->entries[i].type == PPB_CMDLINE_ARG) {
            argc++;
        }
    }

    return argc;
}

void ppb_copy_argv(struct proc_param_block* ppb, size_t ppb_size,
    char** argv, int argc)
{
    int i;
    int arg;

    if (!ppb_is_valid(ppb, ppb_size)) {
        return;
    }

    arg = 0;
    for (i = 0; i < ppb->num; i++) {
        if (ppb->entries[i].type == PPB_CMDLINE_ARG) {
            argv[arg++] = ((char*) ppb) + ppb->entries[i].value;
            if (arg == argc) {
                break;
            }
        }
    }

    argv[argc] = NULL;
}

int ppb_extract_env(struct proc_param_block* ppb, size_t ppb_size)
{
    const char* name;
    const char* value;
    size_t name_len;
    int i;
    int ret;

    if (!ppb_is_valid(ppb, ppb_size)) {
        return -EINVAL;
    }

    for (i = 0; i < ppb->num; i++) {
        if (ppb->entries[i].type == PPB_ENV_VAR) {
            name = ((char*) ppb) + ppb->entries[i].value;
            name_len = strlen(name);
            value = name + name_len + 1;

            ret = setenv(name, value, 1);
            if (ret < 0) {
                return ret;
            }
        }
    }

    return 0;
}

int ppb_forward_streams(struct proc_param_block* ppb, size_t ppb_size,
                        pid_t from, pid_t to)
{
    int i;

    if (!ppb_is_valid(ppb, ppb_size)) {
        return -EINVAL;
    }

    for (i = 0; i < ppb->num; i++) {
        lio_stream_t s, remote;

        if (ppb->entries[i].type != PPB_STREAM) {
            continue;
        }

        if (ppb->entries[i].len != sizeof(lio_stream_t)) {
            return -EFAULT;
        }

        s = *(lio_stream_t*)(((char*) ppb) + ppb->entries[i].value);

        if (lio_stream_origin(s) != from) {
            return -EBADF;
        }

        remote = lio_pass_fd(s, to);
        if (remote < 0) {
            return remote;
        }

        *(lio_stream_t*)(((char*) ppb) + ppb->entries[i].value) = remote;
    }

    return 0;
}

int ppb_get_streams(lio_stream_t* streams, struct proc_param_block* ppb,
                    size_t ppb_size, pid_t from, int num)
{
    int i;
    int found = 0;

    if (!ppb_is_valid(ppb, ppb_size)) {
        return -EINVAL;
    }

    for (i = 0; i < ppb->num; i++) {
        lio_stream_t s;

        if (ppb->entries[i].type != PPB_STREAM) {
            continue;
        }

        if (ppb->entries[i].len != sizeof(lio_stream_t)) {
            return -EFAULT;
        }

        s = *(lio_stream_t*)(((char*) ppb) + ppb->entries[i].value);

        if (lio_stream_origin(s) != from) {
            return -EBADF;
        }

        if (found < num) {
            streams[found++] = s;
        } else {
            lio_close(s);
        }
    }
    while (found < num) {
        streams[found++] = 0;
    }

    return found;
}
