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

#include <stdbool.h>
#include "stdio.h"
#include "rpc.h"
#include "stdlib.h"
#include "dir.h"
#include <string.h>

struct dir_handle* directory_open(const char* path)
{
    struct dir_handle* handle;
    io_resource_t* io_res;

    io_res = lio_compat_open(path, IO_OPEN_MODE_READ | IO_OPEN_MODE_DIRECTORY);
    if (io_res == NULL) {
        return NULL;
    }

    handle = malloc(sizeof(*handle));
    handle->lio1_handle = io_res;

    return handle;
}

int directory_close(struct dir_handle* handle)
{
    int result = 0;

    if (lio_compat_close(handle->lio1_handle)) {
        result = -1;
    }
    free(handle);

    return result;
}

io_direntry_t* directory_read(struct dir_handle* io_res)
{
    int ret;

    if ((io_res == NULL) || (lio_compat_eof(io_res->lio1_handle) != 0)) {
        return NULL;
    }

    io_direntry_t* result = malloc(sizeof(*result));

    ret = lio_compat_read(result, sizeof(*result), 1, io_res->lio1_handle);
    if (ret < 0) {
        free(result);
        return NULL;
    }

    return result;
}

int directory_seek(struct dir_handle* handle, long int offset, int origin)
{
    return lio_compat_seek(handle->lio1_handle, offset, origin);
}

bool directory_create(const char* dirname)
{
    io_resource_t* io_res = lio_compat_open(dirname,
        IO_OPEN_MODE_WRITE | IO_OPEN_MODE_CREATE | IO_OPEN_MODE_TRUNC |
        IO_OPEN_MODE_DIRECTORY);

    if(io_res == NULL) {
        return false;
    }

    lio_compat_close(io_res);
    return true;
}

bool is_directory(const char* dirname)
{
    struct dir_handle* dir = directory_open(dirname);
    if (dir != NULL) {
        directory_close(dir);
        return true;
    } else {
        return false;
    }
}
