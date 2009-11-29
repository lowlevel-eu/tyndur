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

#define BLOCK_SIZE (256 * 1024)

void   overlay_post_open(lostio_filehandle_t* fh);
size_t overlay_read(lostio_filehandle_t*,void*,size_t,size_t);
size_t overlay_write(lostio_filehandle_t*,size_t,size_t,void*);
int    overlay_seek(lostio_filehandle_t* fh, uint64_t offset, int origin);
int    overlay_close(lostio_filehandle_t*);

#define TYPEHANDLE_OVERLAY 255

typehandle_t th_overlay = {
    .id             = TYPEHANDLE_OVERLAY,
    .post_open      = overlay_post_open,
    .seek           = overlay_seek,
    .read           = overlay_read,
    .write          = overlay_write,
    .seek           = overlay_seek,
    .close          = overlay_close,
};

typedef enum {
    OVERLAY_READ_CACHE,
    OVERLAY_COW,
} cache_t;

struct overlay {
    size_t      backing_size;
    void**      blocks;
    cache_t     type;
};

int main(int argc, char* argv[])
{
    lostio_init();
    lostio_type_directory_use();

    lostio_register_typehandle(&th_overlay);

    vfstree_create_node("/cached", TYPEHANDLE_OVERLAY, 0,
        (void*) OVERLAY_READ_CACHE, LOSTIO_FLAG_NOAUTOEOF);
    vfstree_create_node("/cow", TYPEHANDLE_OVERLAY, 0,
        (void*) OVERLAY_COW, LOSTIO_FLAG_NOAUTOEOF);

    init_service_register("ramoverlay");

    while(1) {
        wait_for_rpc();
    }
}

void overlay_post_open(lostio_filehandle_t* fh)
{
    size_t backing_size;
    struct overlay* overlay;
    size_t num_blocks;
    void** blocks;

    // Groesse der unterliegenden Datei
    fseek(fh->source, 0, SEEK_END);
    backing_size = ftell(fh->source);
    fseek(fh->source, 0, SEEK_SET);

    // Blocktabelle anlegen
    num_blocks = (backing_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    blocks = calloc(sizeof(void*), num_blocks);

    // Overlay-Strukur anlegen
    overlay = calloc(1, sizeof(*overlay));
    overlay->backing_size = backing_size;
    overlay->blocks = blocks;
    overlay->type = (cache_t) fh->node->data;

    fh->data = overlay;
}

size_t overlay_read(lostio_filehandle_t* fh, void* buf, size_t bs, size_t nb)
{
    struct overlay* overlay = fh->data;
    size_t blocknum;
    void*  block;
    size_t size, max_size;

    // Blockpointer raussuchen
    if (fh->pos >= overlay->backing_size) {
        return 0;
    }

    blocknum = fh->pos / BLOCK_SIZE;
    block = overlay->blocks[blocknum];

    // Die Requestgroesse darf keine Blockgrenzen ueberschreiten
    size = bs * nb;
    max_size = BLOCK_SIZE - (fh->pos % BLOCK_SIZE);

    if (size > max_size) {
        size = max_size;
    }

    if (fh->pos + size > overlay->backing_size) {
        size = overlay->backing_size - fh->pos;
    }

    // Wenn der Lesecache aktiviert ist, ganzen Block laden
    if ((block == NULL) && (overlay->type == OVERLAY_READ_CACHE)) {
        int ret;
        block = overlay->blocks[blocknum] = malloc(BLOCK_SIZE);
        fseek(fh->source, blocknum * BLOCK_SIZE, SEEK_SET);
        ret = fread(block, 1, BLOCK_SIZE, fh->source);
        if (ret != BLOCK_SIZE) {
            return 0;
        }
    }

    // Daten lesen
    if (block == NULL) {
        fseek(fh->source, fh->pos, SEEK_SET);
        size = fread(buf, 1, size, fh->source);
    } else {
        memcpy(buf, ((uint8_t*) block) + (fh->pos % BLOCK_SIZE), size);
    }

    fh->pos += size;

    // EOF setzen, falls noetig
    if (fh->pos >= overlay->backing_size) {
        fh->flags |= LOSTIO_FLAG_EOF;
    }

    return size;
}

size_t overlay_write(lostio_filehandle_t* fh,size_t bs,size_t nb, void* buf)
{
    struct overlay* overlay = fh->data;
    size_t blocknum;
    void*  block;
    size_t size, max_size;
    size_t ret;

    // Blockpointer raussuchen
    if (fh->pos >= overlay->backing_size) {
        return 0;
    }

    blocknum = fh->pos / BLOCK_SIZE;
    block = overlay->blocks[blocknum];

    // Die Requestgroesse darf keine Blockgrenzen ueberschreiten
    size = bs * nb;
    max_size = BLOCK_SIZE - (fh->pos % BLOCK_SIZE);

    if (size > max_size) {
        size = max_size;
    }

    if (fh->pos + size > overlay->backing_size) {
        size = overlay->backing_size - fh->pos;
    }

    // Copy on Write
    if (block == NULL) {
        size_t offset;

        block = overlay->blocks[blocknum] = malloc(BLOCK_SIZE);

        offset = fh->pos % BLOCK_SIZE;
        if (offset) {
            fseek(fh->source, fh->pos, SEEK_SET);
            ret = fread(block, 1, offset, fh->source);
            if (ret != offset) {
                return 0;
            }
        }

        offset = (fh->pos + size) % BLOCK_SIZE;
        if (offset < BLOCK_SIZE) {
            fseek(fh->source, fh->pos + offset, SEEK_SET);
            ret = fread(((uint8_t*) block) + offset, 1, BLOCK_SIZE - offset,
                fh->source);
            if (ret != BLOCK_SIZE - offset) {
                return 0;
            }
        }
    }

    // Daten schreiben
    memcpy(((uint8_t*) block) + (fh->pos % BLOCK_SIZE), buf, size);

    fh->pos += size;

    return size;
}

int overlay_seek(lostio_filehandle_t* fh, uint64_t offset, int origin)
{
    struct overlay* overlay = fh->data;
    uint64_t new_pos = fh->pos;
    uint64_t size = overlay->backing_size;

    switch (origin)  {
        case SEEK_SET:
            new_pos = offset;
            break;

        case SEEK_CUR:
            new_pos += offset;
            break;

        case SEEK_END:
            new_pos = size;
            break;
    }

    // Position nur aktualisieren, wenn sie nicht ausserhalb des Datentraegers
    // liegt.
    if (new_pos > size) {
        return -1;
    } else if (new_pos == size) {
        // Gegebenen Falles noch EOF setzen
        fh->flags |= LOSTIO_FLAG_EOF;
    } else {
        // Sonst EOF loeschen
        fh->flags &= ~LOSTIO_FLAG_EOF;
    }
    fh->pos = new_pos;
    return 0;
}

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
