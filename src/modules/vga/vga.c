/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Mathias Gottschlag.
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

#include "vga.h"
#include "vgamodes.h"
#include "palette.h"

#include "ports.h"
#include "stdlib.h"
#include "syscall.h"
#include "stdio.h"

#include "driver.h"

char *vidmem = 0;
char *backbuffer = 0;

dword sharedmem = 0;
char *shared_buffer = 0;
int needs_update = 0;

int screen_xres = 0;
int screen_yres = 0;
int screen_bpp = 0;
int modex = 0;


dword vga_get_fb_address(void)
{
    dword address;
    outb(VGA_GC_INDEX, 6);
    address = inb(VGA_GC_DATA);
    //Bits 3 und 4 enthalten die Position des Faqmebuffers
    address = (address >> 2) & 3;
    switch(address)
    {
        case 0:
        case 1:
            address = 0xA0000;
            break;
        case 2:
            address = 0xB0000;
            break;
        case 3:
            address = 0xB8000;
            break;
    }
    return address;
}
void vga_write_regs(byte *regs)
{
    dword i;
    //MISC
    outb(VGA_MISC_WRITE, *regs);
    regs++;
    //Sequencer
    for(i = 0; i < VGA_NUM_SEQ_REGS; i++) {
        outb(VGA_SEQ_INDEX, i);
        outb(VGA_SEQ_DATA, *regs);
        regs++;
    }
    //CRTC Register vorbereiten
    outb(VGA_CRTC_INDEX, 0x03);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) | 0x80);
    outb(VGA_CRTC_INDEX, 0x11);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) & ~0x80);
    regs[0x03] |= 0x80;
    regs[0x11] &= ~0x80;
    //CRTC
    for(i = 0; i < VGA_NUM_CRTC_REGS; i++) {
        outb(VGA_CRTC_INDEX, i);
        outb(VGA_CRTC_DATA, *regs);
        regs++;
    }
    //Graphics Controller
    for(i = 0; i < VGA_NUM_GC_REGS; i++) {
        outb(VGA_GC_INDEX, i);
        outb(VGA_GC_DATA, *regs);
        regs++;
    }
    //Attribute Controller
    for(i = 0; i < VGA_NUM_AC_REGS; i++) {
        (void)inb(VGA_INSTAT_READ);
        outb(VGA_AC_INDEX, i);
        outb(VGA_AC_WRITE, *regs);
        regs++;
    }
    //Palette sichern und Modus einschalten
    (void)inb(VGA_INSTAT_READ);
    outb(VGA_AC_INDEX, 0x20);
}
void vga_set_plane(dword plane)
{
    unsigned char pmask = 1 << ( plane & 3);
    //Ebene lesen
    outb(VGA_GC_INDEX, 4);
    outb(VGA_GC_DATA, plane & 3);
    //Ebene auswählen
    outb(VGA_SEQ_INDEX, 2);
    outb(VGA_SEQ_DATA, pmask);
}

int vga_set(int width, int height, int bpp)
{
    //Aufräumen
    if (sharedmem) {
        close_shared_memory(sharedmem);
        sharedmem = 0;
        shared_buffer = 0;
    }
    if (vidmem) {
        mem_free_physical(vidmem, width * height);
        vidmem = 0;
    }
    if (backbuffer) {
        free(backbuffer);
        backbuffer = 0;
    }

    //Register des Grafikmodus heraussuchen.
    byte *regs = 0;
    switch (width)
    {
        case 320:
            if (height == 200) {
                switch (bpp)
                {
                    case 2:
                        regs = g_320x200x4;
                        break;
                    case 8:
                        regs = g_320x200x256;
                        break;
                    default:
                        return 0;
                }
            } else {
                return 0;
            }
            break;
        case 640:
            if (height == 480) {
                switch (bpp)
                {
                    case 1:
                        regs = g_640x480x2;
                        break;
                    case 4:
                        regs = g_640x480x16;
                        break;
                    default:
                        return 0;
                }
            } else {
                return 0;
            }
            break;
        case 720:
            if (height == 480) {
                switch (bpp)
                {
                    case 4:
                        regs = g_720x480x16;
                        break;
                    default:
                        return 0;
                }
            } else {
                return 0;
            }
            break;
        default:
            return 0;
    }
    //Grafik initialisieren
    vga_write_regs(regs);
    screen_xres = width;
    screen_yres = height;
    screen_bpp = bpp;
    modex = 0;
    vga_set_standard_palette(bpp);
    
    return 1;
}

int reserve_ressources(void)
{
    //registriere Ports
    if (!request_ports(0x3BF, 0x20)) {
        puts("[ VGA ] Konnte Ports nicht belegen!");
        return 0;
    }
    return 1;
}
int init_driver(void)
{
    return 1;
}
int close_driver(void)
{
    if (sharedmem) {
        sharedmem = 0;
        close_shared_memory(sharedmem);
        shared_buffer = 0;
    }
    if (vidmem) {
        mem_free_physical(vidmem, screen_xres * screen_yres * screen_bpp / 8);
        vidmem = 0;
    }
    if (backbuffer) {
        free(backbuffer);
        backbuffer = 0;
    }
    vga_write_regs(g_80x25_text);
    screen_xres = 0;
    screen_yres = 0;
    screen_bpp = 0;
    return 1;

}

dword set_resolution(dword width, dword height, dword bpp)
{
    //Aufräumen
    if (sharedmem) {
        sharedmem = 0;
        close_shared_memory(sharedmem);
        shared_buffer = 0;
    }
    if (vidmem) {
        mem_free_physical(vidmem, width * height * bpp / 8);
        vidmem = 0;
    }
    if (backbuffer) {
        free(backbuffer);
        backbuffer = 0;
    }
    //Grafikregister schreiben
    if (!vga_set(width, height, bpp)) {
        printf("[ VGA ] Konnte Grafikregister nicht setzen.\n");
        return 0;
    }
    //Speicher allokieren
    vidmem = mem_allocate_physical(width * height, vga_get_fb_address(), 0);
    if (!vidmem) {
        printf("[ VGA ] Konnte Videospeicher nicht allokieren.\n");
        close_driver();
        return 0;
    }
    backbuffer = malloc(width * height * bpp / 8);
    if (!backbuffer) {
        printf("[ VGA ] Konnte Backbuffer nicht allokieren.\n");
        close_driver();
        return 0;
    }
    sharedmem = create_shared_memory(width * height * bpp / 8);
    shared_buffer = open_shared_memory(sharedmem);
    printf("[ VGA ] ID: %d, Speicher: %X.\n", sharedmem, (unsigned int) shared_buffer);
    modex = 0;
    return sharedmem;
}
resolution_t get_resolution(void)
{
    resolution_t res;
    res.width = screen_xres;
    res.height = screen_yres;
    res.bpp = screen_bpp;
    return res;
}
resolution_list_t get_possible_resolutions(void)
{
    resolution_list_t list;
    list.res_count = 5;
    list.resolutions = malloc(5 * sizeof(resolution_t));
    list.resolutions[0].width = 640;
    list.resolutions[0].height = 480;
    list.resolutions[0].bpp = 1;
    list.resolutions[1].width = 320;
    list.resolutions[1].height = 200;
    list.resolutions[1].bpp = 2;
    list.resolutions[2].width = 640;
    list.resolutions[2].height = 480;
    list.resolutions[2].bpp = 4;
    list.resolutions[3].width = 720;
    list.resolutions[3].height = 480;
    list.resolutions[3].bpp = 4;
    list.resolutions[4].width = 320;
    list.resolutions[4].height = 200;
    list.resolutions[4].bpp = 8;
    return list;
}
palette_t get_palette(void)
{
    palette_t palette;
    int i;
    switch (screen_bpp)
    {
        case 1:
            palette.color_count = 0;
            palette.colors = 0;
            return palette;
        case 2:
            palette.color_count = 0;
            palette.colors = 0;
            return palette;
        case 4:
            palette.color_count = 16;
            palette.colors = malloc(4 * 16);
            for (i = 0; i < 16; i++) {
                palette.colors[i] = 0xFF000000 + (stdpalette4[i][2] << 16)
                 + (stdpalette4[i][1] << 8) + stdpalette4[i][0];
            }
            return palette;
        case 8:
            palette.color_count = 256;
            palette.colors = malloc(4 * 256);
            for (i = 0; i < 256; i++) {
                palette.colors[i] = 0xFF000000 + (stdpalette8[i][2] << 16)
                 + (stdpalette8[i][1] << 8) + stdpalette8[i][0];
            }
            return palette;
        default:
            palette.color_count = 0;
            palette.colors = 0;
            return palette;
    }
}

void update_screen(void)
{
     if (!shared_buffer) { 
        return;
    }
    if (!backbuffer) { 
        return;
    }
    memcpy(backbuffer, shared_buffer, screen_xres * screen_yres * screen_bpp / 8);
    needs_update = 1;
}

void update_screen_partially(int x, int y, int width, int height)
{
     if (!shared_buffer) { 
        return;
    }
    if (!backbuffer) { 
        return;
    }
    memcpy(backbuffer, shared_buffer, screen_xres * screen_yres * screen_bpp / 8);
    needs_update = 1;
}

void main_loop(void)
{
    while (1) {
        yield();
        if (needs_update) {
            if (!backbuffer) { 
                continue;
            }
            if (!vidmem) {
                continue;
            }
            while(inb(0x3DA) & 0x08);
            while(!(inb(0x3Da)&0x08));
            //int x, y;
            int pagesize;
            switch (screen_bpp)
            {
                case 8:
                    memcpy(vidmem, backbuffer, screen_xres * screen_yres);
                    continue;
                case 4:
                    pagesize = screen_xres * screen_yres / 8;
                    vga_set_plane(0);
                    memcpy(vidmem, backbuffer, pagesize);
                    vga_set_plane(1);
                    memcpy(vidmem, backbuffer + pagesize, pagesize);
                    vga_set_plane(2);
                    memcpy(vidmem, backbuffer + pagesize * 2, pagesize);
                    vga_set_plane(3);
                    memcpy(vidmem, backbuffer + pagesize * 3, pagesize);
                    continue;
                case 2:
                    //TODO
                    continue;
                case 1:
                    //TODO
                    continue;
            }
        }
    }
}
