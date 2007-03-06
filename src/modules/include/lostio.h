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
#ifndef _LOSTIO_H_
#define _LOSTIO_H_
#include "types.h"
#include "collections.h"

#define LOSTIO_TYPES_DIRECTORY  1
#define LOSTIO_TYPES_RAMFILE    128


typedef byte typeid_t;

typedef struct
{
    typeid_t        type;
    char*           name;
    qword           size;
    void*           data;

    list_t*         children;
} vfstree_node_t;

//
typedef struct
{
    dword           id;
    pid_t           pid;
    byte            attr;
    io_resource_t*  source;
    qword           pos;

    vfstree_node_t* node;
} lostio_filehandle_t;

typedef struct
{
    void*           data;
    size_t          size;
} read_hdl_reply;

//
typedef struct
{
    typeid_t        id;
    bool            (*pre_open)(char*, byte, pid_t);
    void            (*post_open)(lostio_filehandle_t*);

    read_hdl_reply  (*read)(lostio_filehandle_t*,size_t,size_t);
    size_t          (*write)(lostio_filehandle_t*,size_t,size_t,void*);
    int             (*seek)(lostio_filehandle_t*,int,int);
    int             (*close)(lostio_filehandle_t*);
} typehandle_t;


//lostio.c
void lostio_init();
void lostio_register_typehandle(typehandle_t* typehandle);

//vfstree.c
bool vfstree_create_node(char* path, typeid_t type, size_t size, void* data);
bool vfstree_delete_node(char* path);

//types
void lostio_type_ramfile_use();
void lostio_type_directory_use();

#endif

