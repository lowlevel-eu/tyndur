/*  
 * Copyright (c) 2007 The LOST Project. All rights reserved.
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

#include "types.h"
#include "ports.h"
#include "stdlib.h"
#include "syscall.h"

#include "stdio.h"

#include "vbe.h"
#include "driver.h"

int *vidmem = 0;
int *backbuffer = 0;
int screen_xres = 0;
int screen_yres = 0;
int screen_bpp = 0;

dword sharedmem = 0;
char *shared_buffer = 0;
int needs_update = 0;
int needs_partial_update = 0;
int update_x = 0;
int update_y = 0;
int update_width = 0;
int update_height = 0;

void vbe_write(unsigned short index, unsigned short value)
{
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA, value);
}


int vbe_set(unsigned short xres, unsigned short yres, unsigned short bpp)
{
    screen_xres = xres;
    screen_yres = yres;
    screen_bpp = bpp;
    
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    vbe_write(VBE_DISPI_INDEX_XRES, xres);
    vbe_write(VBE_DISPI_INDEX_YRES, yres);
    vbe_write(VBE_DISPI_INDEX_BPP, bpp);
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
    
    return 1;
}

int reserve_ressources(void)
{
    /*if (!request_ports(0x3BF, 0x20)) { //vsync?
        puts("[ VESA ] Konnte VGA-Ports nicht belegen!");
        return 0;
    }*/
    if (!request_ports(VBE_DISPI_IOPORT_INDEX, 3)) {
        puts("[ VESA ] Konnte Ports nicht belegen!");
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
    screen_xres = 0;
    screen_yres = 0;
    screen_bpp = 0;
    return 0;
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
        mem_free_physical(vidmem, width * height * screen_bpp / 8);
        vidmem = 0;
    }
    if (backbuffer) {
        free(backbuffer);
        backbuffer = 0;
    }
    if (!vbe_set(width, height, bpp)) {
        close_driver();
        return 0;
    }
    //Speicher allokieren
    vidmem = mem_allocate_physical(width * height * bpp / 8, 0xe0000000, 0);
    if (!vidmem) {
        printf("[ VESA ] Konnte Videospeicher nicht allokieren.\n");
        close_driver();
        return 0;
    }
    backbuffer = malloc(width * height * bpp / 8);
    if (!backbuffer) {
        printf("[ VESA ] Konnte Backbuffer nicht allokieren.\n");
        close_driver();
        return 0;
    }
    sharedmem = create_shared_memory(width * height * bpp / 8);
    shared_buffer = open_shared_memory(sharedmem);
    printf("[ VESA ] ID: %d, Speicher: %X.\n", sharedmem, (unsigned int) shared_buffer);
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
    list.res_count = 3;
    list.resolutions = malloc(3 * sizeof(resolution_t));
    list.resolutions[0].width = 1024;
    list.resolutions[0].height = 786;
    list.resolutions[0].bpp = 32;
    list.resolutions[1].width = 800;
    list.resolutions[1].height = 600;
    list.resolutions[1].bpp = 32;
    list.resolutions[2].width = 640;
    list.resolutions[2].height = 480;
    list.resolutions[2].bpp = 32;
    return list;
}
palette_t get_palette(void)
{
    palette_t palette;
    palette.color_count = 0;
    palette.colors = 0;
    return palette;
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
    if (x < 0) {
        width-=x;
        x = 0;
    }
    if (y < 0) {
        height-=y;
        y = 0;
    }
    if (x + width > screen_xres) {
        width = screen_xres - x;
    }
    if (y + height > screen_yres) {
        height = screen_yres - y;
    }
    int iy;
    for (iy = 0; iy < height; iy++) {
        dword offset = (iy + y) * screen_xres * screen_bpp / 8 + x * screen_bpp / 8;
        memcpy(((byte*)backbuffer) + offset, ((byte*)shared_buffer) + offset,
            width * screen_bpp / 8);
    }
    needs_partial_update = 1;
    update_x = x;
    update_y = y;
    update_width = width;
    update_height = height;
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
            memcpy(vidmem, backbuffer, screen_xres * screen_yres * screen_bpp / 8);
            needs_update = 0;
            needs_partial_update = 0;
        } else if (needs_partial_update) {
            if (!backbuffer) { 
                continue;
            }
            if (!vidmem) {
                continue;
            }
            int iy;
            for (iy = 0; iy < update_height; iy++) {
                dword offset = (iy + update_y) * screen_xres * screen_bpp / 8 + update_x * screen_bpp / 8;
                memcpy(((byte*)vidmem) + offset, ((byte*)backbuffer) + offset,
                    update_width * screen_bpp / 8);
            }
            needs_partial_update = 0;
        }
    }

}
