/*
 * Copyright (c) 2008-2009 Mathias Gottschlag, Janosch Graef, Alexander Siol
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#include <cdi/vesa.h>
#include <cdi/bios.h>
#include <cdi/video.h>
#include <cdi/misc.h>
#include <cdi/mem.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#define DEBUG

int cdi_video_vesa_initialize
    (struct cdi_video_vesa_info_block **vesainfo,
    void (*mode_callback)(int modenum, struct cdi_video_vesa_mode_info *mode))
{

    // Phys. Speicher holen - notwendig, da der Speicher Pagealigned sein muss.
    struct cdi_mem_area* vesainfo_area;

    vesainfo_area = cdi_mem_alloc(sizeof(struct cdi_video_vesa_info_block),
        CDI_MEM_PHYS_CONTIGUOUS | CDI_MEM_DMA_4G | 12);
    if (vesainfo_area == NULL) {
        fprintf(stderr, "cdi_mem_alloc failed.\n");
        return -1;
    }

    *vesainfo = vesainfo_area->vaddr;

    // Register und Speicher vorbereiten
    struct cdi_bios_registers regs;
    memset(&regs, 0, sizeof(regs));
    regs.ax = 0x4F00;
    regs.ds = 0x8000;
    regs.es = 0x8000;
    regs.di = 0x0000;
    memcpy(*vesainfo, "VBE2", 4);

    // Speicherliste vorbereiten
    cdi_list_t memorylist = cdi_list_create();
    struct cdi_bios_memory infomemory;
    infomemory.src = *vesainfo;
    infomemory.dest = 0x80000;
    infomemory.size = sizeof(struct cdi_video_vesa_info_block);
    memorylist = cdi_list_push(memorylist, &infomemory);

    // BIOS aufrufen
    // FIXME Checken ob int10 unterstuetzt wird!
    cdi_bios_int10(&regs, memorylist);

    cdi_list_destroy(memorylist);

#ifdef DEBUG
    stdout = NULL;
    printf("VESA: %c%c%c%c\n", (*vesainfo)->signature[0], (*vesainfo)->signature[1],
        (*vesainfo)->signature[2], (*vesainfo)->signature[3]);
    printf("Version: %d.%d\n", (*vesainfo)->version[1], (*vesainfo)->version[0]);
    printf("Modes: %05X\n", (*vesainfo)->modeptr);
#endif

    int i;
    uint16_t *modeptr = (uint16_t*)((char*)*vesainfo + ((*vesainfo)->modeptr & 0xFFFF));
    for (i = 0; i < 111; i++) {
        if (modeptr[i] == 0xFFFF) {
            break;
        }

#ifdef DEBUG
        printf("---\n");
        printf("Mode: %04X\n", modeptr[i]);
#endif

        struct cdi_video_vesa_mode_info modeinfo;
        cdi_video_vesa_get_mode(modeptr[i], &modeinfo);

        // Call Callback
        if (mode_callback != NULL) {
            (*mode_callback)(modeptr[i], &modeinfo);
        }
    }

    return 0;
}

void cdi_video_vesa_get_mode(int modenum, struct cdi_video_vesa_mode_info* mi)
{
    struct cdi_bios_memory infomemory;
    cdi_list_t memorylist;
    memorylist = cdi_list_create();

    infomemory.src = mi;
    infomemory.dest = 0x80000;
    infomemory.size = sizeof(struct cdi_video_vesa_mode_info);

    cdi_list_push(memorylist, &infomemory);

    // Setup registers/memory
    struct cdi_bios_registers regs;
    memset(&regs, 0, sizeof(regs));
    regs.ax = 0x4F01;
    regs.ds = 0x8000;
    regs.es = 0x8000;
    regs.di = 0x0000;
    regs.cx = modenum;

    cdi_bios_int10(&regs, memorylist);
    cdi_list_destroy(memorylist);

#ifdef DEBUG
    stdout = NULL;
    printf("Mode: %dx%dx%d, Framebuffer %x\n", modeinfo->width, modeinfo->height,
        modeinfo->depth, modeinfo->linearfb);
    printf("Sizes: red: %d, blue: %d, green: %d, reserved %d\n",
        modeinfo->redmasksize, modeinfo->bluemasksize, modeinfo->greenmasksize,
        modeinfo->resmasksize);
    printf("LinearFB: 0x%x\nOffscreenMem: 0x%x\nOffscreenSize: %d\n", modeinfo->linearfb,
        modeinfo->offscreenmem, modeinfo->offscreensize);
#endif
}

int cdi_video_vesa_change_mode(int modenumber)
{
    struct cdi_bios_registers regs;
    memset(&regs, 0, sizeof(regs));
    regs.ax = 0x4F02;
    regs.bx = modenumber | 0xC000;
    cdi_bios_int10(&regs, NULL);
    if (((regs.ax & 0xFF00) != 0x4F00) || (regs.ax & 0xFF)) {
        return -1;
    }

    return 0;
}

