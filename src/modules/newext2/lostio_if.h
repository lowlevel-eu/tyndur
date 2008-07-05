/*
 * Copyright (c) 2008 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
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
#ifndef _LOSTIO_IF_H_
#define _LOSTIO_IF_H_

#include <lostio.h>
#include <stdint.h>
#include <collections.h>

/// LostIO-Typen
#define EXT2_LOSTIO_TYPE_ROOT_DIR 255
#define EXT2_LOSTIO_TYPE_FILE 254
#define EXT2_LOSTIO_TYPE_DIR LOSTIO_TYPES_DIRECTORY

/// Dieser Handler wird bei jedem fopen() auf Fat aufgerufen, da das immer
/// schief gehen wird weil ja die daten nicht direkt auf Root liegen
bool lostio_not_found_handler(char** path, byte args, pid_t pid,
    io_resource_t* pipe_source);

///
bool lostio_pre_open_handler(char** path, byte args, pid_t pid,
    io_resource_t* pipe_source);

size_t lostio_read_handler(lostio_filehandle_t* fh, void* buf, size_t bs,
    size_t bc);

size_t lostio_write_handler(lostio_filehandle_t* fh, size_t bs, size_t bc,
    void* data);

int lostio_seek_handler(lostio_filehandle_t* fh, uint64_t offset, int origin);


// Hilfsfunktionen fuer libext2
int dev_read(uint64_t start, size_t size, void* buffer, void* dev);
int dev_write(uint64_t start, size_t size, const void* buffer, void* dev);
#endif
