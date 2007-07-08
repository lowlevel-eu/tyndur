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

#ifndef VGA_H
#define VGA_H

#include "types.h"

extern char *vidmem;
extern char *backbuffer;

extern int screen_xres;
extern int screen_yres;
extern int screen_bpp;

//Ports
//Attribute Controller
#define VGA_AC_INDEX        0x3C0
#define VGA_AC_WRITE        0x3C0
#define VGA_AC_READ         0x3C1
//
#define VGA_MISC_WRITE      0x3C2
//Sequencer
#define VGA_SEQ_INDEX       0x3C4
#define VGA_SEQ_DATA        0x3C5
//Digital-to-Analog-Converter
#define VGA_DAC_WRITE       0x3C6
#define VGA_DAC_READ_INDEX  0x3C7
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA        0x3C9
//
#define VGA_MISC_READ       0x3CC
//Graphics Controller
#define VGA_GC_INDEX        0x3CE
#define VGA_GC_DATA         0x3CF
//CRT Controller
/*                  COLOR emulation     MONO emulation */
#define VGA_CRTC_INDEX      0x3D4       /* 0x3B4 */
#define VGA_CRTC_DATA       0x3D5       /* 0x3B5 */
//Input
#define VGA_INSTAT_READ     0x3DA

#define VGA_NUM_SEQ_REGS    5
#define VGA_NUM_CRTC_REGS   25
#define VGA_NUM_GC_REGS     9
#define VGA_NUM_AC_REGS     21
#define VGA_NUM_REGS        (1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS + \
    VGA_NUM_GC_REGS + VGA_NUM_AC_REGS)


dword vga_get_fb_address(void);
void vga_write_regs(byte *regs);
void vga_set_plane(dword plane);

int vga_set(int width, int height, int bpp);
int vga_init(void);
int vga_close(void);

#endif
