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


size_t dir_read(lostio_filehandle_t* filehandle, void* buf,
    size_t blocksize, size_t blockcount);

int dir_seek(lostio_filehandle_t* filehandle, uint64_t offset,
    int origin);

/**
 * Die Ramfile-Handler unter einer bestimmten id registrieren, damit sie
 * Benutzbar werden.
 */
void lostio_type_directory_use_as(typeid_t id)
{
    typehandle_t* dir_typehandle = malloc(sizeof(typehandle_t));

    dir_typehandle->id           = id;
    dir_typehandle->not_found    = NULL;
    dir_typehandle->pre_open     = NULL;
    dir_typehandle->post_open    = NULL;
    dir_typehandle->read         = &dir_read;
    dir_typehandle->write        = NULL;
    dir_typehandle->seek         = &dir_seek;
    dir_typehandle->close        = NULL;
    dir_typehandle->link         = NULL;
    dir_typehandle->unlink       = NULL;

    lostio_register_typehandle(dir_typehandle);
}


/**
 * Die Ramfile-Handler registrieren, damit sie Benutzbar werden
 */
void lostio_type_directory_use()
{
    lostio_type_directory_use_as(LOSTIO_TYPES_DIRECTORY);
}


/**
 * Verzeichniseintraege lesen
 */
size_t dir_read(lostio_filehandle_t* filehandle, void* buf,
    size_t blocksize, size_t blockcount)
{
    vfstree_node_t* node;
    size_t size = blockcount;
    int i;

    //Ueberpruefen, ob die angegebene Groesse > Dateigroesse
    if((filehandle->pos + size) > filehandle->node->size)
    {
        size = filehandle->node->size - filehandle->pos;
    }

    size *= sizeof(io_direntry_t);
    io_direntry_t* direntry = (io_direntry_t*) buf;

    for(i = 0; i < (size / sizeof(io_direntry_t)); i++)
    {
        node = list_get_element_at(filehandle->node->children, i + filehandle->pos);
        //printf("[ LOSTIO ] Direntry '%s'\n", node->name);
        direntry->size = node->size;
        //Namen kopieren
        memcpy(direntry->name, node->name, strlen(node->name) + 1);
        //direntry->name[strlen(node->name)] = 0;
        
        if((node->flags & LOSTIO_FLAG_BROWSABLE) != 0)
        {
            direntry->type = IO_DIRENTRY_DIR;
        }
        else
        {
            direntry->type = IO_DIRENTRY_FILE;
        }
        
        
        direntry++;
    }
    
    
    filehandle->pos += size / sizeof(io_direntry_t);
    
    //Auf EOF ueberpruefen
    if(filehandle->pos >= filehandle->node->size) {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    } else {
        filehandle->flags &= ~LOSTIO_FLAG_EOF;
    }


    return size;
}


/**
 *
 */
int dir_seek(lostio_filehandle_t* filehandle, uint64_t offset, int origin)
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


