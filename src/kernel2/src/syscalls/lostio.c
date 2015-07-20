/*
 * Copyright (c) 2009-2011 The tyndur Project. All rights reserved.
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

#include "kernel.h"
#include "kprintf.h"
#include "syscall.h"
#include <syscall_structs.h>

#include "lostio/client.h"
#include "lostio/userspace.h"

/// Ressource suchen
void syscall_lio_resource(const char* path, size_t path_len, int flags,
    lio_usp_resource_t* res_id)
{
    struct lio_resource* res;

    // TODO: is_userspace
    if (path[path_len - 1] != '\0') {
        *res_id = -EINVAL;
        return;
    }

    res = lio_get_resource(path, flags);
    if (res == NULL) {
        *res_id = -ENOENT;
        return;
    }

    *res_id = lio_usp_get_id(res);
}

/// Informationen über eine Ressource abfragen
int syscall_lio_stat(lio_usp_resource_t* resid, struct lio_stat* sbuf)
{
    struct lio_resource* res;

    // TODO: is_userspace
    res = lio_usp_get_resource(*resid);
    if (res == NULL) {
        return -EINVAL;
    }

    // TODO: is_userspace
    return lio_stat(res, sbuf);
}

/// Ressource öffnen
void syscall_lio_open(lio_usp_resource_t* resid, int flags,
    lio_usp_stream_t* stream_id)
{
    struct lio_usp_stream* fd;
    struct lio_stream* stream;
    struct lio_resource* res;

    // TODO: is_userspace
    res = lio_usp_get_resource(*resid);
    if (res == NULL) {
        *stream_id = -EINVAL;
    }

    if (!(stream = lio_open(res, flags))) {
        *stream_id = -EACCES;
        return;
    }

    fd = lio_usp_add_stream(current_process, stream);
    *stream_id = fd->usp_id;
}

/// Stream schließen
int syscall_lio_close(lio_usp_stream_t* stream_id)
{
    struct lio_usp_stream* fd;

    // TODO: is_userspace
    fd = lio_usp_get_stream(current_process, *stream_id);
    if (fd == NULL) {
        return -EBADF;
    }

    return lio_usp_unref_stream(current_process, fd);
}

/// Stream an anderen Prozess weitergeben
int syscall_lio_pass_fd(lio_usp_stream_t* stream_id, pid_t pid)
{
    struct lio_usp_stream* fd;
    struct lio_stream* stream;
    pm_process_t* process;
    int ret;

    process = pm_get(pid);
    if (process == NULL) {
        return -EINVAL;
    }

    // TODO: is_userspace
    fd = lio_usp_get_stream(current_process, *stream_id);
    if (fd == NULL) {
        return -EBADF;
    }

    stream = fd->stream;
    fd = lio_usp_add_stream(process, stream);
    *stream_id = fd->usp_id;

    // Wir haben gerade eine zweite Referenz hinzugefügt, also kann kein
    // lio_close() aufgerufen, das fehlschlagen könnte.
    ret = lio_usp_unref_stream(current_process, fd);
    BUG_ON(ret != 0);

    return 0;
}

/// Aus Stream lesen
void syscall_lio_read(lio_usp_stream_t* stream_id, uint64_t* offset,
    size_t bytes, void* buffer, int flags, ssize_t* result)
{
    struct lio_usp_stream* fd;

    // TODO: is_userspace
    fd = lio_usp_get_stream(current_process, *stream_id);
    if (fd == NULL) {
        *result = -EBADF;
        return;
    }

    if (flags & LIO_REQ_FILEPOS) {
        *result = lio_read(fd->stream, bytes, buffer);
    } else {
        *result = lio_pread(fd->stream, *offset, bytes, buffer);
    }
}

/// In Stream schreiben
void syscall_lio_write(lio_usp_stream_t* stream_id, uint64_t* offset,
    size_t bytes, const void* buffer, int updatepos, ssize_t* result)
{
    struct lio_usp_stream* fd;

    // TODO: is_userspace
    fd = lio_usp_get_stream(current_process, *stream_id);
    if (fd == NULL) {
        *result = -EBADF;
        return;
    }

    if (updatepos) {
        *result = lio_write(fd->stream, bytes, buffer);
    } else {
        *result = lio_pwrite(fd->stream, *offset, bytes, buffer);
    }
}

/// Cursorposition in der Datei ändern
void syscall_lio_seek(lio_usp_stream_t* stream_id, int64_t* offset,
    int whence, int64_t* result)
{
    struct lio_usp_stream* fd;

    // TODO: Ein paar is_userspace
    fd = lio_usp_get_stream(current_process, *stream_id);
    if (fd == NULL) {
        *result = -EBADF;
        return;
    }

    *result = lio_seek(fd->stream, *offset, whence);
}

/// Verzeichnisinhalt auslesen
void syscall_lio_read_dir(lio_usp_resource_t* resid, size_t start, size_t num,
    struct lio_usp_dir_entry* buf, ssize_t* result)
{
    // FIXME: Das d00f
    struct lio_dir_entry* int_buf;
    struct lio_resource* res;
    ssize_t i;

    res = lio_usp_get_resource(*resid);
    if (res == NULL) {
        *result = -EINVAL;
        return;
    }

    int_buf = malloc(num * sizeof(*int_buf));
    // TODO: is_userspace
    *result = lio_read_dir(res, start, num, int_buf);

    for (i = 0; i < *result; i++) {
        buf[i].resource = lio_usp_get_id(int_buf[i].resource);
        strcpy(buf[i].name, int_buf[i].name);
        buf[i].stat = int_buf[i].stat;
    }

    free(int_buf);
}

/// Neue Datei anlegen
void syscall_lio_mkfile(lio_usp_resource_t* parent, const char* name,
    size_t name_len, lio_usp_resource_t* result)
{
    struct lio_resource* parentres;
    struct lio_resource* res;

    parentres = lio_usp_get_resource(*parent);
    if (parentres == NULL) {
        *result = -EINVAL;
        return;
    }

    // TODO: is_userspace
    if (name[name_len - 1] != '\0') {
        *result = -EINVAL;
        return;
    }

    // TODO: is_userspace
    res = lio_mkfile(parentres, name);
    if (res == NULL) {
        *result = -EIO; // FIXME
        return;
    }

    *result = lio_usp_get_id(res);
}

/// Neues Verzeichnis anlegen
void syscall_lio_mkdir(lio_usp_resource_t* parent, const char* name,
    size_t name_len, lio_usp_resource_t* result)
{
    struct lio_resource* parentres;
    struct lio_resource* res;

    parentres = lio_usp_get_resource(*parent);
    if (parentres == NULL) {
        *result = -EINVAL;
        return;
    }

    // TODO: is_userspace
    if (name[name_len - 1] != '\0') {
        *result = -EINVAL;
        return;
    }

    // TODO: is_userspace
    res = lio_mkdir(parentres, name);
    if (res == NULL) {
        *result = -EIO; // FIXME
        return;
    }

    *result = lio_usp_get_id(res);
}

/// Neuen Symlink anlegen
void syscall_lio_mksymlink(lio_usp_resource_t* parent, const char* name,
    size_t name_len, const char* target, size_t target_len,
    lio_usp_resource_t* result)
{
    struct lio_resource* parentres;
    struct lio_resource* res;

    parentres = lio_usp_get_resource(*parent);
    if (parentres == NULL) {
        *result = -EINVAL;
        return;
    }

    // TODO: is_userspace
    if (name[name_len - 1] != '\0') {
        *result = -EINVAL;
        return;
    }

    // TODO: is_userspace
    if (target[target_len - 1] != '\0') {
        *result = -EINVAL;
        return;
    }

    res = lio_mksymlink(parentres, name, target);
    if (res == NULL) {
        *result = -EIO; // FIXME
        return;
    }
    *result = lio_usp_get_id(res);
}

/// Veränderte Blocks schreiben
int syscall_lio_sync(lio_usp_stream_t* stream_id)
{
    struct lio_usp_stream* fd;

    // TODO: is_userspace
    fd = lio_usp_get_stream(current_process, *stream_id);
    if (fd == NULL) {
        return -EBADF;
    }

    return lio_sync(fd->stream);
}

/// Dateigröße ändern
int syscall_lio_truncate(lio_usp_stream_t* stream_id, uint64_t* size)
{
    struct lio_usp_stream* fd;

    // TODO: is_userspace
    fd = lio_usp_get_stream(current_process, *stream_id);
    if (fd == NULL) {
        return -EBADF;
    }

    return lio_truncate(fd->stream, *size);
}

/// Verzeichniseintrag löschen
int syscall_lio_unlink(lio_usp_resource_t* parent, const char* name,
    size_t name_len)
{
    struct lio_resource* parentres;

    parentres = lio_usp_get_resource(*parent);
    if (parentres == NULL) {
        return -EINVAL;
    }

    // TODO: is_userspace
    if (name[name_len - 1] != '\0') {
        return -EINVAL;
    }

    // TODO: is_userspace
    return lio_unlink(parentres, name);
}

/// Alle veränderten Blocks im Cache rausschreiben
int syscall_lio_sync_all(int soft)
{
    return lio_sync_all(soft);
}
