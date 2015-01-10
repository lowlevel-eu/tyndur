/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
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

#include <init.h>
#include <lostio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <errno.h>

#define BLOCK_SIZE (256 * 1024)

typedef enum {
    OVERLAY_READ_CACHE,
    OVERLAY_COW,
} cache_t;

struct overlay {
    size_t      backing_size;
    void**      blocks;
    cache_t     type;
};

struct lio_resource* overlay_load_root(struct lio_tree* tree)
{
    int64_t backing_size;
    struct overlay* overlay;
    size_t num_blocks;
    struct lio_resource* root;
    struct lio_resource* cached_res;
    struct lio_resource* cow_res;
    void** blocks;

    // Groesse der unterliegenden Datei
    backing_size = lio_seek(tree->source, 0, SEEK_END);
    if (backing_size < 0) {
        return NULL;
    }

    // Blocktabelle anlegen
    num_blocks = (backing_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    blocks = calloc(sizeof(void*), num_blocks);

    // Wurzelknoten
    root = lio_resource_create(tree);
    root->browsable = 1;
    root->blocksize = BLOCK_SIZE;
    lio_resource_modified(root);

    // 'cached'
    overlay = malloc(sizeof(*overlay));
    *overlay = (struct overlay) {
        .backing_size   = backing_size,
        .blocks         = blocks,
        .type           = OVERLAY_READ_CACHE,
    };

    cached_res = lio_resource_create(tree);
    cached_res->readable = 1;
    cached_res->writable = 1;
    cached_res->size = backing_size;
    cached_res->blocksize = 1;
    cached_res->opaque = overlay;

    lio_resource_modified(cached_res);
    lio_node_create(root, cached_res, "cached");

    // 'cow'
    overlay = malloc(sizeof(*overlay));
    *overlay = (struct overlay) {
        .backing_size   = backing_size,
        .blocks         = blocks,
        .type           = OVERLAY_COW,
    };

    cow_res = lio_resource_create(tree);
    cow_res->readable = 1;
    cow_res->writable = 1;
    cow_res->size = backing_size;
    cow_res->blocksize = 1;
    cow_res->opaque = overlay;

    lio_resource_modified(cow_res);
    lio_node_create(root, cow_res, "cow");

    return root;
}

int overlay_read_partial(struct lio_resource* res, uint64_t offset,
                         size_t bytes, void* buf)
{
    struct overlay* overlay = res->opaque;
    size_t blocknum;
    void*  block;
    size_t size, max_size;

    // Blockpointer raussuchen
    if (offset >= overlay->backing_size) {
        return -EIO;
    }

    blocknum = offset / BLOCK_SIZE;
    block = overlay->blocks[blocknum];

    // Die Requestgroesse darf keine Blockgrenzen ueberschreiten
    size = bytes;
    max_size = BLOCK_SIZE - (offset % BLOCK_SIZE);

    if (size > max_size) {
        size = max_size;
    }

    if (offset + size > overlay->backing_size) {
        size = overlay->backing_size - offset;
    }

    // Wenn der Lesecache aktiviert ist, ganzen Block laden
    if ((block == NULL) && (overlay->type == OVERLAY_READ_CACHE)) {
        int ret;
        size_t block_size;

        block = overlay->blocks[blocknum] = malloc(BLOCK_SIZE);

        if ((blocknum + 1) * BLOCK_SIZE > overlay->backing_size) {
            block_size = overlay->backing_size % BLOCK_SIZE;
        } else {
            block_size = BLOCK_SIZE;
        }

        ret = lio_pread(res->tree->source, blocknum * BLOCK_SIZE, block_size,
                        block);
        if (ret != block_size) {
            return -EIO;
        }
    }

    // Daten lesen
    if (block == NULL) {
        return lio_pread(res->tree->source, offset, size, buf);
    } else {
        memcpy(buf, ((uint8_t*) block) + (offset % BLOCK_SIZE), size);
        return size;
    }
}

int overlay_read(struct lio_resource* res, uint64_t offset, size_t bytes,
                 void* buf)
{
    int ret;

    while (bytes > 0) {
        ret = overlay_read_partial(res, offset, bytes, buf);
        if (ret < 0) {
            return ret;
        }

        offset += ret;
        bytes -= ret;
        buf = ((uint8_t*) buf) + ret;
    }

    return 0;
}

int overlay_write_partial(struct lio_resource* res, uint64_t offset,
                          size_t bytes, void* buf)
{
    struct overlay* overlay = res->opaque;
    size_t blocknum;
    void*  block;
    size_t size, max_size;
    size_t ret;

    // Blockpointer raussuchen
    if (offset >= overlay->backing_size) {
        return -ENOSPC;
    }

    blocknum = offset / BLOCK_SIZE;
    block = overlay->blocks[blocknum];

    // Die Requestgroesse darf keine Blockgrenzen ueberschreiten
    size = bytes;
    max_size = BLOCK_SIZE - (offset % BLOCK_SIZE);

    if (size > max_size) {
        size = max_size;
    }

    if (offset + size > overlay->backing_size) {
        size = overlay->backing_size - offset;
    }

    // Copy on Write
    if (block == NULL) {
        block = overlay->blocks[blocknum] = malloc(BLOCK_SIZE);

        ret = lio_pread(res->tree->source, blocknum * BLOCK_SIZE, BLOCK_SIZE,
                        block);
        if (ret != BLOCK_SIZE) {
            return -EIO;
        }
    }

    // Daten schreiben
    memcpy(((uint8_t*) block) + (offset % BLOCK_SIZE), buf, size);

    return size;
}

int overlay_write(struct lio_resource* res, uint64_t offset, size_t bytes,
                  void* buf)
{
    int ret;

    while (bytes > 0) {
        ret = overlay_write_partial(res, offset, bytes, buf);
        if (ret < 0) {
            return ret;
        }

        offset += ret;
        bytes -= ret;
        buf = ((uint8_t*) buf) + ret;
    }

    return 0;
}

#if 0
// TODO close/unload mit LIOv2
int overlay_close(lostio_filehandle_t* fh)
{
    struct overlay* overlay = fh->data;
    size_t i, num_blocks;

    num_blocks = (overlay->backing_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (i = 0; i < num_blocks; i++) {
        free(overlay->blocks[i]);
    }

    free(overlay->blocks);
    free(overlay);

    return 0;
}
#endif


static struct lio_service service = {
    .name               = "ramoverlay",
    .lio_ops = {
        .load_root      = overlay_load_root,
        .read           = overlay_read,
        .write          = overlay_write,
    },
};

int main(int argc, char* argv[])
{
    lio_add_service(&service);
    init_service_register("ramoverlay");

    while(1) {
        lio_dispatch();
        lio_srv_wait();
    }
}
