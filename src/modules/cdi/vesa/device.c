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

#include "device.h"
#include <cdi/video.h>
#include <cdi/vesa.h>
#include <cdi/lists.h>
//#include <cdi/pci.h>
#include <cdi/mem.h>
#include <cdi/misc.h>
#include <cdi/bios.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vesa.h"
#include "drawing.h"
#include "bitmap.h"
#include <unistd.h>

struct vesa_device* vesa_device_create(char* name, struct cdi_pci_device* pci)
{
    // Device anlegen
    struct vesa_device* dev;
    dev = malloc(sizeof(struct vesa_device));
    dev->dev.dev.name = name;
    dev->dev.displays = cdi_list_create();

    dev->dev.raster_op = CDI_VIDEO_ROP_COPY;
    dev->dev.restore_data = NULL;

    dev->dev.vram = NULL;

    //int i;

    // Ressourcen holen (Speicher als Framebuffer)
    // FIXME: Hack, Keine Ahnung ob das außerhalb von qemu tut.

    /*
    cdi_pci_alloc_memory(pci);
    for (i = 0; i < cdi_list_size(pci->resources); i++) {
        struct cdi_pci_resource* res;
        res = cdi_list_get(pci->resources, i);
        if (res->type == CDI_PCI_MEMORY && res->length > 0x1000) {
            dev->dev.vram = res->address;
            dev->dev.vram_size = res->length;
            break;
        }
    }
    */

    struct cdi_bios_registers regs;
    memset(&regs, 0, sizeof(regs));
    regs.ax = 0x0F00;
    cdi_bios_int10(&regs, NULL);

    dev->dev.textmode = regs.ax & 0xFF;

    // Display anlegen
    struct cdi_video_display* vesadisplay;
    vesadisplay = calloc(1,sizeof(struct cdi_video_display));

    vesadisplay->device = (struct cdi_video_device*)dev;
    vesadisplay->mode = NULL;
    vesadisplay->modes = cdi_list_create();

    // Muss bei change_mode sinnvoll gesetzt werden
    vesadisplay->frontbuffer = NULL;

    cdi_list_push(dev->dev.displays, vesadisplay);

    return dev;
}

/**
 * Callback fuer CDI-Vesa-Modi
 */
void vesa_mode_callback(int modenum, struct cdi_video_vesa_mode_info *modeinfo)
{
    switch (modeinfo->depth) {
        case 32:
        case 24:
            printf("Modus OK: %dx%dx%d, memory_model: %d\n", modeinfo->width,
                modeinfo->height, modeinfo->depth, modeinfo->type);
            struct cdi_video_display *vesadisplay;
            struct vesa_device *vesadev;
            // Geraet & Display suchen.
            // VESA kann nur ein Geraet und ein Display
            vesadev = cdi_list_get(vesa_driver.drv.devices, 0);
            vesadisplay = cdi_list_get(vesadev->dev.displays, 0);

            // Modus hinzufuegen
            struct cdi_video_vesa_mode *vesamode;
            vesamode = calloc(1,sizeof(struct cdi_video_vesa_mode));
            vesamode->vesamode = modenum;
            vesamode->mode.width = modeinfo->width;
            vesamode->mode.height = modeinfo->height;
            vesamode->mode.depth = modeinfo->depth;
            cdi_list_push(vesadisplay->modes, vesamode);

            printf("  Color Info:\n    bits: red %d | green %d | blue %d\n",
                modeinfo->redmasksize, modeinfo->greenmasksize,
                modeinfo->bluemasksize);
            printf("    offsets: red %d | green %d | blue %d\n",
                modeinfo->redfieldpos, modeinfo->greenfieldpos,
                modeinfo->bluefieldpos);

            size_t calculated_pitch = modeinfo->width * modeinfo->depth / 8;
            printf(" calc pitch: %d | real pitch: %d\n", calculated_pitch, modeinfo->pitch);

            break;
        default:
            /*
             Andere Modi sind einfach zu ungenau um die wirklich in Erwaegung zu
             ziehen, auch wenn die technisch moeglich sein sollten
             */
            break;
    }
    return;
}

/**
 * Modus setzen
 */
int vesa_display_set_mode(struct cdi_video_device* device,
    struct cdi_video_display* display, struct cdi_video_displaymode* mode)
{
    struct cdi_video_vesa_mode* vesamode;
    vesamode = (struct cdi_video_vesa_mode*)mode;
    cdi_video_vesa_change_mode(vesamode->vesamode);

    struct cdi_video_bitmap* bitmap;
    if (display->frontbuffer != NULL) {
        bitmap = display->frontbuffer;
    } else {
        bitmap = calloc(1, sizeof(struct cdi_video_bitmap));
    }

    struct cdi_video_vesa_mode_info vesamodeinfo;
    cdi_video_vesa_get_mode(vesamode->vesamode, &vesamodeinfo);

    bitmap->width = mode->width;
    bitmap->height = mode->height;
    bitmap->format.bpp = vesamodeinfo.depth;
    bitmap->format.red_bits = vesamodeinfo.redmasksize;
    bitmap->format.red_offset = vesamodeinfo.redfieldpos;
    bitmap->format.green_bits = vesamodeinfo.greenmasksize;
    bitmap->format.green_offset = vesamodeinfo.greenfieldpos;
    bitmap->format.blue_bits = vesamodeinfo.bluemasksize;
    bitmap->format.blue_offset = vesamodeinfo.bluefieldpos;
    bitmap->format.alpha_bits = 0;
    bitmap->format.alpha_offset = 0;

    // Offsets muessen auf 32-bit passen.
    if (bitmap->format.bpp == 16) {
        bitmap->format.red_offset += 16;
        bitmap->format.green_offset += 16;
        bitmap->format.blue_offset += 16;
    }

//    if (bitmap->format.bpp == 24) {
//        bitmap->format.red_offset += 8;
//        bitmap->format.green_offset += 8;
//        bitmap->format.blue_offset += 8;
//    }


    bitmap->device = device;
    bitmap->pitch = vesamodeinfo.pitch;
//    bitmap->in_vram = 1;
//    bitmap->pixeldata = NULL; // Im VRAM, passt so

    stdout = 0;
    printf("linearfb: 0x%08x\n", vesamodeinfo.linearfb);

    // Alten Speicher ggf. vorher freigeben
    if (device->vram) {
        printf("Pre-Free-Vram\n");
        // FIXME: kernel2 unterstützt MEM_FREE_PHYSICAL noch nicht!
        // cdi_mem_free(device->vram);
        printf("Post-Free-Vram\n");
        device->vram = NULL;
    }

    // FIXME Nach display oder device verschieben.
//    struct cdi_mem_area* mem;
    device->vram = cdi_mem_map(vesamodeinfo.linearfb,
                               bitmap->height * bitmap->pitch);

//    bitmap->in_vram = 0;
//    bitmap->pixeldata = mem->vaddr;

    bitmap->buffercount = 1;
    bitmap->buffers = calloc(1, sizeof(struct cdi_video_bitmap_buffer));

    // FIXME Eigentlich ist das ja im vram... jedenfalls ist free() eine Doofe Idee.
    bitmap->buffers[0].in_vram = 1;
    bitmap->buffers[0].pixeldata = 0;
    bitmap->buffers[0].type = CDI_VIDEO_BUFFER_READ | CDI_VIDEO_BUFFER_WRITE | CDI_VIDEO_BUFFER_DISPLAY;

    // FIXME
    printf("mem_area: 0x%08x\nvaddr: 0x%08x | paddr: FIXME\n", device->vram, device->vram->vaddr);

    display->frontbuffer = bitmap;

#if 1
    // Die Sichere Variante um das Bitmap zu schwaerzen
    struct cdi_video_color color;
    color.alpha = 0;
    color.red = 0xff;
    color.green = 0x00;
    color.blue = 0x00;

    vesa_draw_rectangle(bitmap, 0, 0, bitmap->width, bitmap->height, &color);
#endif

#if 0
    // Inkorrekte Variante, aber schneller - und macht effektiv das gleiche.
    memset(bitmap->pixeldata, 0, bitmap->width * bitmap->height *
        bitmap->format.bpp / 8);
#endif
    // Standardformat setzen
    memcpy(&pxformat, &(bitmap->format), sizeof(struct cdi_video_pixel_format));

    // Debug
    stdout = 0;

    // Bitmaps konvertieren
    int i, size;
    size = cdi_list_size(vesa_bitmap_list);
    for (i = 0; i < size; i++) {
        struct cdi_video_bitmap* convert_bitmap;
        convert_bitmap = cdi_list_get(vesa_bitmap_list, i);
        printf("bitmap: 0x%x\n", convert_bitmap);
        printf("w: %d | h: %d | bpp: %d\n", convert_bitmap->width, convert_bitmap->height, convert_bitmap->format.bpp);
        //vesa_bitmap_convert(convert_bitmap, bitmap->format);
    }

    return 0;
}

/**
 * Modus setzen
 */
int vesa_restore_text_mode(struct cdi_video_device* device)
{
    struct cdi_bios_registers regs;
    memset(&regs, 0, sizeof(regs));
    regs.ax = device->textmode;
    cdi_bios_int10(&regs, NULL);
    stdout=0;
    printf("(DRV) Try Setting Text Mode\n");
    if ((regs.ax & 0xFF) == device->textmode) {
        printf("Success.\n");
        return 0;
    } else {
        return -1;
    }
}
