/*
 * Copyright (c) 2007 The LOST Project. All rights reserved.
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
#include "types.h"
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "io.h"
#include "collections.h"
#include "lostio.h"

typehandle_t typehandle;

read_hdl_reply  read(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount);
size_t          write(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount, void* data);
int             seek(lostio_filehandle_t* filehandle , int offset, int origin);

/**
 * Die Ramfile-Handler registrieren, sodass sie Benutzbar werden
 */
void lostio_type_ramfile_use()
{
    typehandle.id           = LOSTIO_TYPES_RAMFILE;
    typehandle.pre_open     = NULL;
    typehandle.post_open    = NULL;
    typehandle.read         = &read;
    typehandle.write        = &write;
    typehandle.seek         = &seek;
    typehandle.close        = NULL;

    lostio_register_typehandle(&typehandle);
}


/**
 *
 */
read_hdl_reply read(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount)
{
    read_hdl_reply reply;
    reply.data = filehandle->node->data;
    reply.size = filehandle->node->size;
    return reply;
}


/**
 *
 */
size_t write(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount, void* data)
{
    puts("ramfile write");
}


/**
 *
 */
int seek(lostio_filehandle_t* filehandle , int offset, int origin)
{
    puts("ramfile seek");
}


