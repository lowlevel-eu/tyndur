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

#include "kprintf.h"
#include "syscall.h"
#include <syscall_structs.h>

/// Ressource suchen
void syscall_lio_resource(const char* path, size_t path_len, int flags,
    lio_usp_resource_t* res_id)
{
    *res_id = -1;
}

/// Informationen über eine Ressource abfragen
int syscall_lio_stat(lio_usp_resource_t* resid, struct lio_stat* sbuf)
{
    return -ENOSYS;
}

/// Ressource öffnen
void syscall_lio_open(lio_usp_resource_t* resid, int flags,
    lio_usp_stream_t* stream_id)
{
    *stream_id = -1;
}

/// Stream schließen
int syscall_lio_close(lio_usp_stream_t* stream_id)
{
    return -ENOSYS;
}

/// Aus Stream lesen
void syscall_lio_read(lio_usp_stream_t* stream_id, uint64_t* offset,
    size_t bytes, void* buffer, int updatepos, ssize_t* result)
{
    *result = -1;
}

/// In Stream schreiben
void syscall_lio_write(lio_usp_stream_t* stream_id, uint64_t* offset,
    size_t bytes, const void* buffer, int updatepos, ssize_t* result)
{
    *result = -1;
}

/// Cursorposition in der Datei ändern
void syscall_lio_seek(lio_usp_stream_t* stream_id, int64_t* offset,
    int whence, int64_t* result)
{
    *result = -1;
}

/// Verzeichnisinhalt auslesen
void syscall_lio_read_dir(lio_usp_resource_t* resid, size_t start, size_t num,
    struct lio_usp_dir_entry* buf, ssize_t* result)
{
    *result = -1;
}

/// Neue Datei anlegen
void syscall_lio_mkfile(lio_usp_resource_t* parent, const char* name,
    size_t name_len, lio_usp_resource_t* result)
{
    *result = -1;
}

/// Neues Verzeichnis anlegen
void syscall_lio_mkdir(lio_usp_resource_t* parent, const char* name,
    size_t name_len, lio_usp_resource_t* result)
{
    *result = -1;
}

/// Neuen Symlink anlegen
void syscall_lio_mksymlink(lio_usp_resource_t* parent, const char* name,
    size_t name_len, const char* target, size_t target_len,
    lio_usp_resource_t* result)
{
    *result = -1;
}

/// Veränderte Blocks schreiben
int syscall_lio_sync(lio_usp_stream_t* stream_id)
{
    return -ENOSYS;
}

/// Dateigröße ändern
int syscall_lio_truncate(lio_usp_stream_t* stream_id, uint64_t* size)
{
    return -ENOSYS;
}

/// Verzeichniseintrag löschen
int syscall_lio_unlink(lio_usp_resource_t* parent, const char* name,
    size_t name_len)
{
    return -ENOSYS;
}

/// Alle veränderten Blocks im Cache rausschreiben
int syscall_lio_sync_all(int soft)
{
    return -ENOSYS;
}

