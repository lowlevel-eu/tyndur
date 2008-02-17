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
#ifndef _IO_STRUCT_H_
#define _IO_STRUCT_H_
#include <stdint.h>
#include <sys/types.h>

#define MAX_FILENAME_LEN 255
#define IO_DIRENTRY_FILE 1
#define IO_DIRENTRY_DIR 2

#define IO_OPEN_MODE_READ 1
#define IO_OPEN_MODE_WRITE 2
#define IO_OPEN_MODE_APPEND 4
#define IO_OPEN_MODE_TRUNC 8
#define IO_OPEN_MODE_DIRECTORY 16
#define IO_OPEN_MODE_CREATE 32
#define IO_OPEN_MODE_LINK 64
#define IO_OPEN_MODE_SYNC 128

typedef uint32_t io_resource_id_t;
typedef uint8_t io_direntry_type_t;
typedef struct
{
    io_resource_id_t    id;
    char*               path;
    pid_t               pid;
    io_resource_id_t    resid;

    void*               buffer_ptr;
    size_t              buffer_size;
    size_t              buffer_pos;
    uint8_t             buffer_mode;

    size_t              ungetc_count;
    uint8_t*            ungetc_buffer;
} __attribute__ ((packed)) io_resource_t;
#endif //ifndef _IO_STRUCT_H_

