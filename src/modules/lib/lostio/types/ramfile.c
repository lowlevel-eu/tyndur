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
#include "types.h"
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "io.h"
#include "collections.h"
#include "lostio.h"

size_t ramfile_read(lostio_filehandle_t* filehandle, void* buf,
    size_t blocksize, size_t blockcount);
size_t ramfile_write(lostio_filehandle_t* filehandle,
    size_t blocksize, size_t blockcount, void* data);

int ramfile_seek(lostio_filehandle_t* filehandle, uint64_t offset, int origin);

/**
 * Die Ramfile-Handler registrieren, sodass sie benutzbar werden
 */
void lostio_type_ramfile_use()
{
    lostio_type_ramfile_use_as(LOSTIO_TYPES_RAMFILE);
}

/**
 * Die Ramfile-Handler registrieren, sodass sie unter einer bestimmten ID 
 * benutzbar werden.
 */
void lostio_type_ramfile_use_as(typeid_t id)
{
    typehandle_t* ramfile_typehandle = malloc(sizeof(typehandle_t));
    ramfile_typehandle->id           = id;
    ramfile_typehandle->not_found    = NULL;
    ramfile_typehandle->pre_open     = NULL;
    ramfile_typehandle->post_open    = NULL;
    ramfile_typehandle->read         = &ramfile_read;
    ramfile_typehandle->write        = &ramfile_write;
    ramfile_typehandle->seek         = &ramfile_seek;
    ramfile_typehandle->close        = NULL;
    ramfile_typehandle->link         = NULL;
    ramfile_typehandle->unlink       = NULL;

    lostio_register_typehandle(ramfile_typehandle);
}


/**
 * Aus einer Ramfile lesen
 */
size_t ramfile_read(lostio_filehandle_t* filehandle, void* buf,
    size_t blocksize, size_t blockcount)
{
    size_t remaining = filehandle->node->size - filehandle->pos;
    size_t size;

    //Ueberpruefen, ob die angegebene Groesse > Dateigroesse
    if(blocksize * blockcount > remaining) {
        size = remaining;
    } else {
        size = blocksize * blockcount;
    }

    memcpy(buf, filehandle->node->data + filehandle->pos, size);

    filehandle->pos += size;

    //Auf EOF ueberpruefen
    if(filehandle->pos == filehandle->node->size) {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    }

    return size;
}


/**
 *
 */
size_t ramfile_write(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount, void* data)
{
    //Ueberpruefen, ob die angegebene Groesse > Dateigroesse
    if(blocksize * blockcount > filehandle->node->size)
    {
        //Datei vergroessern
        filehandle->node->data = realloc(filehandle->node->data, blocksize * blockcount);
        filehandle->node->size = blocksize * blockcount;
    }
    
    
    memcpy(filehandle->node->data, data, blocksize * blockcount);
    
    filehandle->pos += blocksize * blockcount;
    
    //Auf EOF ueberpruefen
    if(filehandle->pos == filehandle->node->size)
    {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    }

    return blocksize * blockcount;
}


/**
 *
 */
int ramfile_seek(lostio_filehandle_t* filehandle, uint64_t offset, int origin)
{
    switch(origin)
    {
        case SEEK_SET:
        {
            if(offset > filehandle->node->size)
            {
                return -1;
            }

            filehandle->pos = offset;
            break;
        }

        case SEEK_CUR:
        {
            if((offset + filehandle->pos) > filehandle->node->size)
            {
                return -1;
            }
            filehandle->pos += offset;
            break;
        }

        case SEEK_END:
        {
            filehandle->pos = filehandle->node->size;
            break;
        }
    }
    
    //Auf EOF ueberpruefen
    if(filehandle->pos == filehandle->node->size)
    {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    }
    else
    {
        filehandle->flags &= ~LOSTIO_FLAG_EOF;
    }

    return 0;
}

