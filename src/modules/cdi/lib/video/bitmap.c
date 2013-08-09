/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Alexander Siol.
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
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#include "bitmap.h"
#include <cdi/video.h>
#include <types.h>
#include <stdlib.h>
#include <syscall.h>

cdi_list_t bitmap_list = NULL;
uint64_t bitmapid_ctr = 0;

void bitmaps_initialize(void)
{
    bitmap_list = cdi_list_create();
}

interfacebitmap_t* bitmap_create(struct cdi_video_bitmap* bitmap, pid_t pid)
{
    interfacebitmap_t* interfacebitmap;
    interfacebitmap = calloc(1, sizeof(interfacebitmap_t));
    interfacebitmap->bitmap = bitmap;
    interfacebitmap->id = ++bitmapid_ctr;
    interfacebitmap->owner = pid;
    interfacebitmap->upload_buffer_id = 0;
    interfacebitmap->upload_buffer_size = 0;
    interfacebitmap->upload_buffer_ptr = NULL;

    cdi_list_push(bitmap_list, interfacebitmap);

    return interfacebitmap;
}

void bitmap_destroy(interfacebitmap_t *bitmap)
{
    int i;
    for (i = 0; i < cdi_list_size(bitmap_list); i++) {
        if (cdi_list_get(bitmap_list, i) == bitmap) {
            cdi_list_remove(bitmap_list, i);
        }
    }
    // SHM vorher schliessen
    if (bitmap->upload_buffer_id != 0) {
        close_shared_memory(bitmap->upload_buffer_id);
    }
    free(bitmap);
}

interfacebitmap_t* bitmap_get_by_id(uint64_t id)
{
    interfacebitmap_t* bitmap;
    int i;
    for (i = 0; i < cdi_list_size(bitmap_list); i++) {
        bitmap = cdi_list_get(bitmap_list, i);
        if (bitmap->id == id) {
            return bitmap;
        }
    }
    return NULL;
}
