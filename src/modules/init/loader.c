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

#include <stdbool.h>
#include <syscall.h>
#include <loader.h>
#include <lock.h>
#include <init.h>
#include <errno.h>
#include <lostio.h>
#include <stdlib.h>

static struct {
    pid_t   pid;
    char*   args;
    pid_t   parent_pid;
    lock_t  lock;
} loader_state;

size_t ainflate(void* src, size_t len, void** dest_ptr);

/**
 * Speicher allozieren um ihn spaeter in einen neuen Prozess zu mappen. Diese
 * Funktion sollte nicht fuer "normale" Allokationen benutzt werden, da immer
 * ganze Pages alloziert werden.
 *
 * @param size minimale Groesse des Bereichs
 *
 * @return Adresse, oder NULL falls ein Fehler aufgetreten ist
 */
vaddr_t loader_allocate_mem(size_t size)
{
    return mem_allocate(size, 0);
}

/**
 * Ein Stueck Speicher in den Prozess mappen. Dieser darf dazu noch nicht
 * gestartet sein. Der Speicher muss zuerst mit loader_allocate_mem alloziert
 * worden sein, denn sonst kann nicht garantiert werden, dass der Speicher
 * uebertragen werden kann.
 *
 * @param process PID des Prozesses
 * @param dest_address Adresse an die der Speicher im Zielprozess soll
 * @param src_address Adresse im aktuellen Kontext die uebetragen werden soll
 * @param size Groesse des Speicherbereichs in Bytes
 *
 * @return true, wenn der bereich gemappt wurde, FALSE sonst
 */
bool loader_assign_mem(pid_t process, vaddr_t dest_address,
    vaddr_t src_address, size_t size)
{
    init_child_page(loader_state.pid, dest_address, src_address, size);
    return true;
}

/**
 * Lädt eine Shared Library in den Speicher.
 *
 * @param name Name der Shared Library
 * @param image Enthält bei Erfolg einen Pointer auf ein Binärimage
 * @param size Enthält bei Erfolg die Größe des Binärimages in Bytes
 *
 * @return 0 bei Erfolg, -errno im Fehlerfall
 */
int loader_get_library(const char* name, void** image, size_t* size)
{
    io_resource_t* fd;
    char* path;
    void* p;
    int ret;

    ret = asprintf(&path, "file:/system/%s.so", name);
    if (ret < 0) {
        return -ENOMEM;
    }

    fd = lio_compat_open(path, IO_OPEN_MODE_READ);
    free(path);
    if (fd == NULL) {
        return -ENOENT;
    }

    if (!lio_compat_seek(fd, 0, SEEK_END)) {
        ret = -EIO;
        goto fail;
    }

    ret = lio_compat_tell(fd);
    if (ret < 0) {
        goto fail;
    }
    *size = ret;

    if (!lio_compat_seek(fd, 0, SEEK_SET)) {
        ret = -EIO;
        goto fail;
    }

    p = malloc(*size);
    if (p == NULL) {
        ret = -ENOMEM;
        goto fail;
    }

    ret = lio_compat_read(p, 1, *size, fd);
    if (ret < 0) {
        free(p);
        goto fail;
    }

    if (*(uint16_t*) p == 0x8b1f) {
        /* gzip, erstmal entpacken */
        void* buf;
        size_t buf_len;

        buf_len = ainflate(p, *size, &buf);
        if (buf_len == 0) {
            ret = -EINVAL;
            goto fail;
        }

        free(p);
        p = buf;
        *size = buf_len;
    }

    *image = p;
    ret = 0;
fail:
    lio_compat_close(fd);
    return ret;
}

/**
 * Erstellt einen neuen Thread.
 *
 * @param process PID
 * @param address Einsprungsadresse des Threads
 *
 * @return bool true, wenn der Thread erstellt wurde, FALSE sonst
 */
bool loader_create_thread(pid_t process, vaddr_t address)
{
    loader_state.pid = create_process((uintptr_t) address, 0,
        loader_state.args, loader_state.parent_pid);
    return loader_state.pid != 0;
}

/**
 * Erstellt einen neuen Prozess aus einer ELF-Datei
 */
pid_t load_single_module(void* image, size_t image_size, char* args,
    pid_t parent_pid, int ppb_shm_id)
{
    p();
    lock(&loader_state.lock);

    loader_state.args = args;
    loader_state.parent_pid = parent_pid;
    if (!loader_elf32_load_image(0, image, image_size)) {
        loader_state.pid = 0;
        goto fail;
    }

    if (ppb_shm_id) {
        void *ppb = open_shared_memory(ppb_shm_id);
        ssize_t ppb_size = shared_memory_size(ppb_shm_id);

        if (!ppb || ppb_size < 0) {
            goto fail;
        }

        ppb_forward_streams(ppb, ppb_size, parent_pid, loader_state.pid);
        close_shared_memory(ppb_shm_id);

        init_child_ppb(loader_state.pid, ppb_shm_id);
    }

    unblock_process(loader_state.pid);

fail:
    unlock(&loader_state.lock);
    v();
    return loader_state.pid;
}
