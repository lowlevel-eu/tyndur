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

#include <cdi/video.h>
#include <cdi/mem.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "drawing.h"

struct cdi_video_pixel_format pxformat = {
    .bpp = 32,
    .red_bits = 8,
    .red_offset = 16,
    .green_bits = 8,
    .green_offset = 8,
    .blue_bits = 8,
    .blue_offset = 0,
};

cdi_list_t vesa_bitmap_list;


struct cdi_video_bitmap* vesa_bitmap_create(struct cdi_video_device* device,
    unsigned int width, unsigned int height, void* data)
{
    struct cdi_video_bitmap* bitmap = calloc(1,sizeof(struct cdi_video_bitmap));

    bitmap->device = device;
    bitmap->width = width;
    bitmap->height = height;

    // Videoformat uebernehmen, nicht statisch festlegen
    // Gleiches Format erlaubt mehr Optimierungen
    memcpy(&(bitmap->format), &pxformat, sizeof(struct cdi_video_pixel_format));

    bitmap->pitch = bitmap->format.bpp / 8 * width;

    bitmap->buffercount = 1;

    bitmap->buffers = calloc(1, sizeof(struct cdi_video_bitmap_buffer));
    bitmap->buffers[0].in_vram = 0;
    bitmap->buffers[0].pixeldata = calloc(1, width * height * bitmap->format.bpp);
    bitmap->buffers[0].type = CDI_VIDEO_BUFFER_WRITE | CDI_VIDEO_BUFFER_READ;

//    bitmap->in_vram = 0;

//    bitmap->pixeldata = calloc(1, width * height * bitmap->format.bpp);
    if (data) {
        //memcpy(bitmap->pixeldata, data, width * height * bitmap->format.bpp);
        // FIXME Entfernen!
    }

    cdi_list_push(vesa_bitmap_list, bitmap);

    printf("created bitmap at 0x%x\n", bitmap);
    return bitmap;
}

void vesa_bitmap_destroy(struct cdi_video_bitmap* bitmap)
{
    int i = 0;
    int size = cdi_list_size(vesa_bitmap_list);
    for (i = 0; i < size; i++) {
        if (cdi_list_get(vesa_bitmap_list, i) == bitmap) {
            cdi_list_remove(vesa_bitmap_list, i);
            break;
        }
    }

    for (i = 0; i < bitmap->buffercount; i++) {
        if (bitmap->buffers[i].in_vram == 0 && bitmap->buffers[i].pixeldata != NULL) {
            free(bitmap->buffers[i].pixeldata);
        }
    }
    free(bitmap->buffers);

//    if (bitmap->in_vram == 0) {
//        free(bitmap->pixeldata);
//    }
    stdout = 0;
    printf("Freed bitmap at 0x%x\n", bitmap);
    free(bitmap);
}

void vesa_bitmap_set_usage_hint(struct cdi_video_device* device,
    struct cdi_video_bitmap* bitmap, enum cdi_video_bitmap_usage_hint hint)
{
    /*
     Meiner Meinung nach kann sowas mit VESA garnicht gescheit gemacht werden -
     immerhin sind eh alle Bitmaps im genau gleichen RAM.
     */
    return;
}

void vesa_bitmap_convert(struct cdi_video_bitmap* bitmap,
    struct cdi_video_pixel_format newfmt)
{
    stdout = 0;
    if (memcmp(&newfmt, &(bitmap->format),
        sizeof(struct cdi_video_pixel_format)) == 0)
    {
        // Nichts zu tun, Format gleich geblieben.
        printf("No Convert this time.\n");
        return;
    }

    printf("Convert Bitmap, prev: %d(%d)-%d(%d)-%d(%d), new: %d(%d)-%d(%d)-%d(%d)\n",
        bitmap->format.red_bits, bitmap->format.red_offset, bitmap->format.green_bits, bitmap->format.green_offset, bitmap->format.blue_bits, bitmap->format.blue_offset, newfmt.red_bits, newfmt.red_offset, newfmt.green_bits, newfmt.green_offset, newfmt.blue_bits, newfmt.blue_offset);
    printf("bitmap: 0x%x\n", bitmap);
    printf("w: %d | h: %d | bpp: %d\n", bitmap->width, bitmap->height, bitmap->format.bpp);
    struct cdi_video_bitmap tempbmp;
    memcpy(&tempbmp, bitmap, sizeof(struct cdi_video_bitmap));
    memcpy(&(tempbmp.format), &newfmt, sizeof(struct cdi_video_pixel_format));
//    tempbmp.in_vram = 0;
//    tempbmp.pixeldata = calloc(1, tempbmp.width * tempbmp.height * tempbmp.format.bpp);
//    tempbmp.pitch = tempbmp.format.bpp / 8 * tempbmp.width;
//    vesa_draw_bitmap(&tempbmp, bitmap, 0, 0);
//    if (bitmap->in_vram == 0) {
//        free(bitmap->pixeldata);
//    }
//    memcpy(bitmap, &tempbmp, sizeof(struct cdi_video_bitmap));
}

void vesa_bitmap_flip_buffers(struct cdi_video_bitmap* bitmap)
{
    stdout = NULL;

    int x;
    int write_buffer = -1;
    int display_buffer = -1;
    int unused_buffer = -1;

    size_t size = bitmap->pitch * bitmap->height;

    // Nur ein Buffer? NOP!
    if (bitmap->buffercount == 1) {
        return;
    }

    // Altes READ-Flag entfernen, WRITE suchen.
    for (x = 0; x < bitmap->buffercount; x++) {
        //printf("Buffers old flags: %d: %d\n", x, bitmap->buffers[x].type);
        bitmap->buffers[x].type &= ~CDI_VIDEO_BUFFER_READ;
        if (bitmap->buffers[x].type == 0) {
            bitmap->buffers[x].type |= CDI_VIDEO_BUFFER_UNUSED;
        }
        if (bitmap->buffers[x].type & CDI_VIDEO_BUFFER_WRITE) {
            write_buffer = x;
        }
        if (bitmap->buffers[x].type & CDI_VIDEO_BUFFER_DISPLAY) {
            display_buffer = x;
        }
        //printf("Buffers new flags: %d: %d\n", x, bitmap->buffers[x].type);
    }

    if (write_buffer == -1) {
        printf("Epic Fail: No Write Buffer %d\n", write_buffer);
        return;
    }

    // Der fertig geschriebene Buffer wird neuer Lesebuffer, Schreiben entfernen
    bitmap->buffers[write_buffer].type |= CDI_VIDEO_BUFFER_READ;
    bitmap->buffers[write_buffer].type &= ~CDI_VIDEO_BUFFER_WRITE;

    // Falls DISPLAY-Buffer vorhanden, diesen aktualisieren
    if (display_buffer != -1) {
        char* disp_addr = NULL;
        if (bitmap->buffers[display_buffer].in_vram) {
            disp_addr = bitmap->device->vram->vaddr;
        }
        disp_addr += (uint32_t)bitmap->buffers[display_buffer].pixeldata;
        memcpy(disp_addr, bitmap->buffers[write_buffer].pixeldata, size);
        // FIXME potentiell boese (reservierte bits/bytes)
    }

    // Unbenutzten Buffer suchen.
    for (x = 0; x < bitmap->buffercount; x++) {
        if (bitmap->buffers[x].type & CDI_VIDEO_BUFFER_UNUSED) {
            unused_buffer = x;
            break;
        }
    }

    // Unbenutzten Buffer verwenden
    if (unused_buffer != -1) {
        bitmap->buffers[unused_buffer].type &= ~CDI_VIDEO_BUFFER_UNUSED;
        bitmap->buffers[unused_buffer].type |= CDI_VIDEO_BUFFER_WRITE;
    }
    // Sonst den alten Buffer weiter benutzen
    else {
        bitmap->buffers[write_buffer].type |= CDI_VIDEO_BUFFER_WRITE;

        // Falls DISPLAY-Buffer vorhanden, den zum lesen verwenden.
        // Sollte faktisch immer der Fall sein, falls wir mehr als einen Buffer
        // haben
        if (display_buffer != -1) {
            bitmap->buffers[display_buffer].type |= CDI_VIDEO_BUFFER_READ;
            bitmap->buffers[write_buffer].type &= ~CDI_VIDEO_BUFFER_READ;
        }
    }
}

int vesa_bitmap_request_backbuffers(struct cdi_video_bitmap* bitmap, int backbuffercount)
{
    // Mehr als Triple-Buffer bei Software-Rendern? Waere ziemlicher Mist.
    if (backbuffercount > 2) {
        backbuffercount = 2;
    }

    // Frontbuffer gibts auch noch
    int buffercount = backbuffercount + 1;

    if (buffercount == bitmap->buffercount) {
        // NOP, aendert sich ja nix.
        return backbuffercount;
    }

    // Evtl. ueberzaehlige Buffer sauber entfernen
    int x;
    for (x = buffercount; x < bitmap->buffercount; x++) {
        // Speicher in RAM freigeben
        if (bitmap->buffers[x].in_vram == 0) {
            free(bitmap->buffers[x].pixeldata);
        }
    }

    // Neue Bufferbereiche alloziieren
    size_t buffers_size = buffercount * sizeof(struct cdi_video_bitmap_buffer);
    bitmap->buffers = realloc(bitmap->buffers, buffers_size);

    // Neue Pixeldata-Buffer alloziieren
    size_t pixeldata_size = bitmap->pitch * bitmap->height;

    for (x = bitmap->buffercount; x < buffercount; x++) {
        bitmap->buffers[x].in_vram = 0;
        bitmap->buffers[x].pixeldata = malloc(pixeldata_size);
        bitmap->buffers[x].type = 0;

        memset(bitmap->buffers[x].pixeldata, 0, pixeldata_size);
    }

    bitmap->buffercount = buffercount;

    // Alle Typen zuruecksetzen
    for (x = 0; x < bitmap->buffercount; x++) {
        // Alle Typen ausser DISPLAY entfernen
        bitmap->buffers[x].type &= CDI_VIDEO_BUFFER_DISPLAY;
    }

    // Typen neu initialisieren
    if (bitmap->buffercount == 1) {
        bitmap->buffers[0].type |= CDI_VIDEO_BUFFER_READ;
        bitmap->buffers[0].type |= CDI_VIDEO_BUFFER_WRITE;
    }
    else if (bitmap->buffercount == 2) {
        bitmap->buffers[0].type |= CDI_VIDEO_BUFFER_READ;
        bitmap->buffers[1].type |= CDI_VIDEO_BUFFER_WRITE;
    }
    else {
        bitmap->buffers[1].type |= CDI_VIDEO_BUFFER_READ;
        bitmap->buffers[2].type |= CDI_VIDEO_BUFFER_WRITE;
    }

    // UNUSED setzen wo noetig.
    for (x = 0; x < bitmap->buffercount; x++) {
        if (bitmap->buffers[x].type == 0) {
            bitmap->buffers[x].type |= CDI_VIDEO_BUFFER_UNUSED;
        }
    }

    return backbuffercount;
}
