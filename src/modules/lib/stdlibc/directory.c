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
#include <lostio.h>
#include <errno.h>

static struct dir_handle* new_opendir(const char* path)
{
    struct dir_handle* handle = NULL;
    lio_resource_t res;
    char* abspath = io_get_absolute_path(path);

    res = lio_resource(abspath, 1);
    if (res < 0) {
        goto out;
    }

    /* Prüfen, ob es auch wirklich ein Verzeicnis ist */
    if (lio_read_dir(res, 0, 0, NULL) != 0) {
        goto out;
    }

    handle = malloc(sizeof(*handle));
    handle->old = false;
    handle->new_res = res;
    handle->offset = 0;

out:
    free(abspath);
    return handle;
}

struct dir_handle* directory_open(const char* path)
{
    struct dir_handle* handle;
    io_resource_t* io_res;

    handle = new_opendir(path);
    if (handle) {
        return handle;
    }

    io_res = lio_compat_open(path, IO_OPEN_MODE_READ | IO_OPEN_MODE_DIRECTORY);
    if (io_res == NULL) {
        return NULL;
    }

    handle = malloc(sizeof(*handle));
    handle->old = true;
    handle->lio1_handle = io_res;

    return handle;
}

static int new_closedir(struct dir_handle* handle)
{
    free(handle);
    return 0;
}
int directory_close(struct dir_handle* handle)
{
    int result = 0;

    if (!handle->old) {
        return new_closedir(handle);
    }

    if (lio_compat_close(handle->lio1_handle)) {
        result = -1;
    }
    free(handle);

    return result;
}

static io_direntry_t* new_directory_read(struct dir_handle* handle)
{
    io_direntry_t* ent = NULL;
    struct lio_dir_entry lent;

    if (lio_read_dir(handle->new_res, handle->offset, 1, &lent) == 1) {
        ent = calloc(sizeof(*ent), 1);
        strcpy(ent->name, lent.name);
        ent->type =
            (((lent.stat.flags & LIO_FLAG_READABLE) |
                (lent.stat.flags & LIO_FLAG_WRITABLE)) ? IO_DIRENTRY_FILE : 0)|
            ((lent.stat.flags & LIO_FLAG_BROWSABLE) ? IO_DIRENTRY_DIR : 0) |
            ((lent.stat.flags & LIO_FLAG_RESOLVABLE) ? IO_DIRENTRY_LINK : 0);
        ent->size = lent.stat.size;

        handle->offset++;
    }

    return ent;
}

io_direntry_t* directory_read(struct dir_handle* io_res)
{
    int ret;

    if (!io_res->old) {
        return new_directory_read(io_res);
    }

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
    if (!handle->old) {
        switch (origin) {
            case SEEK_SET:
                break;
            case SEEK_CUR:
                offset += (handle->offset * sizeof(io_direntry_t));
                break;
            case SEEK_END:
            default:
                return -1;
        }
        handle->offset = offset / sizeof(io_direntry_t);
        return offset;
    }

    return lio_compat_seek(handle->lio1_handle, offset, origin);
}

static bool new_directory_create(const char* path)
{
    char* abs = io_get_absolute_path(path);
    char* dir = io_split_dirname(abs);
    char* file = io_split_filename(abs);
    bool result = false;
    lio_resource_t parent, ret;

    if ((parent = lio_resource(dir, 1)) >= 0) {
        ret = lio_mkdir(parent, file);
        result = (ret >= 0);
        if (!result) {
            errno = -ret;
        }
    }

    free(dir);
    free(file);
    free(abs);

    return result;
}

static bool old_directory_create(const char* dirname)
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

bool directory_create(const char* dirname)
{
    return new_directory_create(dirname) || old_directory_create(dirname);
}

static bool new_is_directory(const char* path)
{
    lio_resource_t res = lio_resource(path, 1);

    if (res < 0) {
        return false;
    }

    return (lio_read_dir(res, 0, 0, NULL) == 0);
}

static bool old_is_directory(const char* path)
{
    struct dir_handle* dir = directory_open(path);
    if (dir != NULL) {
        directory_close(dir);
        return true;
    } else {
        return false;
    }
}

bool is_directory(const char* dirname)
{
    return new_is_directory(dirname) || old_is_directory(dirname);
}

