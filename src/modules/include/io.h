/*  
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
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
#ifndef _IO_H_
#define _IO_H_

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <io_struct.h>

typedef struct
{
    char                name[MAX_FILENAME_LEN];
    io_direntry_type_t  type;
    uint64_t            size;
    time_t              ctime;
    time_t              mtime;
    time_t              atime;

} __attribute__ ((packed)) io_direntry_t;



#ifdef MODULE_INIT
    void io_init(void);

    void rpc_io_open(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);
#else
    typedef struct
    {
        io_resource_id_t id;
        size_t blocksize;
        size_t blockcount;
        uint32_t shared_mem_id;
    } __attribute__ ((packed)) io_read_request_t;

    typedef struct
    {
        io_resource_id_t id;
        size_t blocksize;
        size_t blockcount;
        uint32_t shared_mem_id;
        uint8_t data[];
    } __attribute__ ((packed)) io_write_request_t;

    typedef struct
    {
        io_resource_id_t id;
        uint64_t offset;
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
    
    typedef struct
    {
        io_resource_id_t target_id;
        io_resource_id_t dir_id;

        size_t name_len;
        char name[];
    } __attribute__ ((packed)) io_link_request_t;
    
    typedef struct
    {
        io_resource_id_t dir_id;

        size_t name_len;
        char name[];
    } __attribute__ ((packed)) io_unlink_request_t;
#endif

int io_create_link(const char* target_path, const char* link_path,
    bool hardlink);
int io_remove_link(const char* link_path);

char* io_get_absolute_path(const char* path);
char* io_split_filename(const char* path);
char* io_split_dirname(const char* path);

#endif //ifndef _IO_H_
