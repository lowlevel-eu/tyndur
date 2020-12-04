/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "kprintf.h"

#include "lostio/core.h"
#include "lostio/client.h"
#include "lostio_int.h"

#define BLOCKSIZE 1024

static struct lio_resource root_res = {
    .size = 0,
    .blocksize = BLOCKSIZE,
    .changeable = 1,
    .browsable = 1,
    .children = NULL,
    .opaque = NULL,
};
static struct lio_resource dev_root_res = {
    .size = 0,
    .blocksize = BLOCKSIZE,
    .changeable = 1,
    .browsable = 1,
    .children = NULL,
    .opaque = NULL,
};


static struct lio_service service;
static struct lio_service dev_service;

struct tmp_state {
    void*   buf;
    size_t  size;
};

static struct lio_resource* load_root(struct lio_tree* tree)
{
    struct lio_resource* res;

    if (tree->source) {
        return NULL;
    }

    if (tree->service == &service) {
        res = &root_res;
    } else if (tree->service == &dev_service) {
        res = &dev_root_res;
    } else {
        return NULL;
    }

    lio_init_resource(res);
    return res;
}

static int load_children(struct lio_resource* res)
{
    return 0;
}

static int tmp_read(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf)
{
    struct tmp_state* s = res->opaque;

    if (bytes % BLOCKSIZE) {
        bytes += BLOCKSIZE - (bytes % BLOCKSIZE);
    }

    if (s->buf && (offset < res->size) && (offset < s->size)) {
        if (bytes > (s->size - offset)) {
            size_t total = bytes;
            bytes = s->size - offset;
            memset((char*) buf + bytes, 0, total - bytes);
        }
        memcpy(buf, s->buf + offset, bytes);
    } else {
        memset(buf, 0, bytes);
    }

    return 0;
}

static int tmp_write(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf)
{
    struct tmp_state* s = res->opaque;

    if (res->size <= offset) {
        /* Hinter das Dateiende wird nicht geschrieben */
        return -EIO;
    } else if (s->size < offset + bytes) {
        /* Der Puffer muss vergroessert werden */
        void* p = realloc(s->buf, offset + bytes);
        if (p == NULL) {
            return -ENOMEM;
        }
        s->buf = p;
        s->size = offset + bytes;
    }

    memcpy(s->buf + offset, buf, bytes);

    return 0;
}

static struct lio_resource* make_file(struct lio_resource* parent,
    const char* name)
{
    struct lio_resource* res;
    struct tmp_state* s;

    res = lio_create_resource();
    lio_create_node(parent, res, name);

    res->blocksize = BLOCKSIZE;
    res->readable = 1;
    res->writable = 1;
    res->seekable = 1;
    res->tree = parent->tree;

    s = calloc(1, sizeof(*s));
    res->opaque = s;

    return res;
}

static struct lio_resource* make_dir(struct lio_resource* parent,
    const char* name)
{
    struct lio_resource* res;

    res = lio_create_resource();
    lio_create_node(parent, res, name);

    res->blocksize = BLOCKSIZE;
    res->browsable = 1;
    res->changeable = 1;
    res->tree = parent->tree;

    return res;
}

static struct lio_resource* make_symlink(struct lio_resource* parent,
    const char* name, const char* target)
{
    struct lio_resource* res;
    struct tmp_state* s;

    res = lio_create_resource();
    lio_create_node(parent, res, name);

    res->blocksize = BLOCKSIZE;
    res->resolvable = 1;
    res->retargetable = 1;
    res->tree = parent->tree;

    s = calloc(1, sizeof(*s));
    s->buf = strdup(target);
    s->size = strlen(s->buf);

    res->opaque = s;
    res->size = s->size;

    return res;
}

static int unlink(struct lio_resource* parent, const char* name)
{
    struct lio_node* child;
    struct lio_resource* res;

    if (!(child = lio_resource_get_child(parent, name))) {
        return -ENOENT;
    }
    res = child->res;

    // Ressource hat noch Kinder
    if (res->browsable && list_size(res->children)) {
        return -EEXIST;
    }

    lio_sync_blocks(res, 0, 0);
    lio_resource_remove_child(parent, child);
    lio_destroy_resource(res);
    return 0;
}

#define TMP_LIO_OPS \
    {                                       \
        .load_root      = load_root,        \
        .load_children  = load_children,    \
        .read           = tmp_read,         \
        .write          = tmp_write,        \
        .make_file      = make_file,        \
        .make_dir       = make_dir,         \
        .make_symlink   = make_symlink,     \
        .unlink         = unlink,           \
    }

static struct lio_service service = {
    .name = "tmp",
    .lio_ops = TMP_LIO_OPS,
};

static struct lio_service dev_service = {
    .name = "dev",
    .lio_ops = TMP_LIO_OPS,
};

void lio_init_tmp(void)
{
    struct lio_resource* dev_root;

    lio_add_service(&service);

    lio_add_service(&dev_service);
    dev_root = lio_get_resource("dev:/", false);
    if (dev_root != NULL) {
        struct lio_resource* dev_fs_res;
        lio_mkdir(dev_root, "fs", &dev_fs_res);
    }
}
