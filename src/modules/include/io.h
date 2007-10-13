/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
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
#ifndef _IO_H_
#define _IO_H_
#include "types.h"


//#define DIRENTRY_NAME_LEN 32 //FIXME: Nur eine uebergangsloesung
#define MAX_FILENAME_LEN 255
#define IO_DIRENTRY_FILE 1
#define IO_DIRENTRY_DIR 2

#define IO_OPEN_MODE_READ 1
#define IO_OPEN_MODE_WRITE 2
#define IO_OPEN_MODE_APPEND 4
#define IO_OPEN_MODE_BINARY 8
#define IO_OPEN_MODE_DIRECTORY 16
#define IO_OPEN_MODE_CREATE 32
#define IO_OPEN_MODE_LINK 64
#define IO_OPEN_MODE_SYNC 128

typedef dword io_resource_id_t;
typedef byte io_direntry_type_t;
typedef struct
{
    io_resource_id_t    id;
    pid_t               pid;
    qword               resid;

    void*               buffer_ptr;
    size_t              buffer_size;
    size_t              buffer_pos;
    byte                buffer_mode;

    size_t              ungetc_count;
    byte*               ungetc_buffer;
} __attribute__ ((packed)) io_resource_t;


typedef struct
{
    char                name[MAX_FILENAME_LEN];
    io_direntry_type_t  type;
    qword               size;
    timestamp_t         ctime;
    timestamp_t         mtime;
    timestamp_t         atime;

} __attribute__ ((packed)) io_direntry_t;



#ifdef MODULE_INIT
    void io_init();

    void rpc_io_open(pid_t pid, dword correlation_id, size_t data_size, void* data);
#else
    typedef struct
    {
        io_resource_id_t id;
        size_t blocksize;
        size_t blockcount;
        dword shared_mem_id;
    } __attribute__ ((packed)) io_read_request_t;

    typedef struct
    {
        io_resource_id_t id;
        size_t blocksize;
        size_t blockcount;
        dword shared_mem_id;
        byte data[];
    } __attribute__ ((packed)) io_write_request_t;

    typedef struct
    {
        io_resource_id_t id;
        long int offset;
        int origin;
    } __attribute__ ((packed)) io_seek_request_t;

    typedef struct
    {
        io_resource_id_t id;
    } __attribute__ ((packed)) io_eof_request_t;


    typedef struct
    {
        io_resource_id_t id;
    } __attribute__ ((packed)) io_tell_request_t;

#endif

char* io_get_absolute_path(const char* path);

#endif //ifndef _IO_H_
