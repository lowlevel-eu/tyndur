/*
 * Copyright (c) 2010 The tyndur Project. All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
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

#include <string.h>
#include <stdlib.h>

#include "lostio_int.h"
#include "kprintf.h"

#define BLOCKSIZE 128

static struct lio_resource* load_root(struct lio_tree* tree);
static int file_read(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf);
static int file_write(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf);
static int truncate(struct lio_resource* res, uint64_t size);

static struct lio_resource root_res = {
    .size = 0,
    .blocksize = BLOCKSIZE,
    .resolvable = 1,
    .retargetable = 1,
    .opaque = NULL,
};

static char* link = NULL;
static size_t link_len = 0;

static struct lio_resource* load_root(struct lio_tree* tree)
{
    if (tree->source) {
        return NULL;
    }

    lio_init_resource(&root_res);
    return &root_res;
}

static int file_read(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf)
{
    if (link) {
        if (bytes > link_len) {
            memset(buf + link_len, 0, bytes - link_len);
            bytes = link_len;
        }
        memcpy(buf, link + offset, bytes);
    }

    return 0;
}

static int file_write(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf)
{
    if (offset + bytes >= link_len) {
        truncate(res, offset + bytes);
    }

    if (link) {
        memcpy(link + offset, buf, bytes);
    }

    return 0;
}

static int truncate(struct lio_resource* res, uint64_t size)
{
    link_len = size;
    link = realloc(link, size);
    memset(link, 0, size);
    return 0;
}

static struct lio_service service = {
    .name = "file",
    .lio_ops = {
        .load_root  = load_root,
        .read       = file_read,
        .write      = file_write,
        .truncate   = truncate,
    },
};

void lio_init_file(void)
{
    lio_add_service(&service);
}
