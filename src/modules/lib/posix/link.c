/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
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

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <syscall.h>
#include <io.h>
#include <lostio.h>
#include <stdlib.h>

/**
 * Einen Hardlink erstellen.
 *
 * @param oldpath Ziel des Links
 * @param newpath Hier wird der Link erstellt
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int link(const char* oldpath, const char* newpath)
{
    // FIXME: Im Moment werden noch keine Hardlinks untestuetzt von LostIO
    errno = EPERM;
    return -1;
}

/**
 * Einen symbolischen Link anlegen
 *
 * @param oldpath Ziel des Links
 * @param newpath Hier wird der Link erstellt
 *
 * @return 0 bei Erfolg; im Fehlerfall -1 und errno wird gesetzt
 */
int symlink(const char* oldpath, const char* newpath)
{
    return io_create_link(oldpath, newpath, 0);
}

static ssize_t lio2_readlink(const char* path, char* buf, size_t bufsize)
{
    lio_stream_t s;
    lio_resource_t r;
    char* full_path;
    ssize_t result;

    if (!(full_path = io_get_absolute_path(path))) {
        errno = ENOMEM;
        return -1;
    }

    r = lio_resource(full_path, 0);
    free(full_path);
    if (r < 0) {
        return -1;
    }

    if ((s = lio_open(r, LIO_SYMLINK | LIO_READ)) < 0) {
        return -1;
    }

    result = lio_read(s, bufsize, buf);
    lio_close(s);

    return result;
}


static ssize_t lio_readlink(const char* path, char* buf, size_t bufsize)
{
    ssize_t len;
    io_resource_t* file;

    file = lio_compat_open(path, IO_OPEN_MODE_READ | IO_OPEN_MODE_LINK);
    if (file == NULL) {
        errno = EIO;
        return -1;
    }

    len = lio_compat_read(buf, 1, bufsize, file);
    lio_compat_close(file);

    return len;
}

/**
 * Liest den Zielpfad eines symbolischen Links aus.
 *
 * Der ausgelesene Pfad wird nicht nullterminiert. Wenn der Zielpfad laenger
 * als die Puffergroesse ist, wird der Pfad abgeschnitten.
 *
 * @param path Pfad zum symbolischen Link
 * @param buf Puffer, in dem der Zielpfad gespeichert werden soll
 * @param bufsiz Laenge des Puffers
 *
 * @return Laenge des Zielpfads; -1 im Fehlerfall.
 */
ssize_t readlink(const char* path, char* buf, size_t bufsize)
{
    ssize_t len;

    if ((len = lio2_readlink(path, buf, bufsize)) < 0) {
        len = lio_readlink(path, buf, bufsize);
    }

    return len;
}
