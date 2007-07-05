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

#include "palette.h"
#include "vga.h"

#include "ports.h"

byte stdpalette8[256][3] =
{
    //Grautöne
    {0, 0, 0},
    {36, 36, 36},
    {73, 73, 73},
    {109, 109, 109},
    {146, 146, 146},
    {182, 182, 182},
    {219, 219, 219},
    {255, 255, 255},
    //Rot
    {64, 0, 0},
    {128, 0, 0},
    {191, 0, 0},
    {255, 0, 0},
    {64, 32, 32},
    {128, 64, 64},
    {191, 96, 96},
    {255, 128, 128},
    //Gelb
    {64, 64, 0},
    {128, 128, 0},
    {191, 191, 0},
    {255, 255, 0},
    {64, 64, 32},
    {128, 128, 64},
    {191, 191, 96},
    {255, 255, 128},
    //Grün
    {0, 64, 0},
    {0, 128, 0},
    {0, 191, 0},
    {0, 255, 0},
    {32, 64, 32},
    {64, 128, 64},
    {96, 191, 96},
    {128, 255, 128},
    //Cyan
    {0, 64, 64},
    {0, 128, 128},
    {0, 191, 191},
    {0, 255, 255},
    {32, 64, 64},
    {64, 128, 128},
    {96, 191, 191},
    {128, 255, 255},
    //Blau
    {0, 0, 64},
    {0, 0, 128},
    {0, 0, 191},
    {0, 0, 255},
    {32, 32, 64},
    {64, 64, 128},
    {96, 96, 191},
    {128, 128, 255},
    //Violett
    {64, 0, 64},
    {128, 0, 128},
    {191, 0, 191},
    {255, 0, 255},
    {64, 32, 64},
    {128, 64, 128},
    {191, 96, 191},
    {255, 128, 255}
};

byte stdpalette4[16][3] =
{
    {0, 0, 0},
    {0, 0, 255},
    {0, 255, 0},
    {0, 128, 128},
    {255, 0, 0},
    {255, 0, 255},
    {128, 128, 0},
    {192, 192, 192},
    {128, 128, 128},
    {128, 128, 255},
    {128, 255, 128},
    {255, 128, 128},
    {255, 128, 255},
    {128, 255, 255},
    {255, 255, 128},
    {255, 255, 255}
};

void vga_set_palette_color(int index, byte r, byte g, byte b)
{
    //Einzelne Farbe in den DAC schreiben
    outb(VGA_DAC_WRITE, 0xff); //Alle Farben maskieren (nur eine wird benötigt)
    outb(VGA_DAC_WRITE_INDEX, index);
    outb(VGA_DAC_DATA, r >> 2); //Nur die 6 niedrigen Bits werden beachtet
    outb(VGA_DAC_DATA, g >> 2);
    outb(VGA_DAC_DATA, b >> 2);
}
void vga_set_standard_palette(int bpp)
{
    int i;
    switch (bpp)
    {
        case 1:
            break;
        case 2:
            break;
        case 4:
            for (i = 0; i < 16; i++) {
                vga_set_palette_color(i, stdpalette4[i][0], stdpalette4[i][1],
                                      stdpalette4[i][2]);
            }
            break;
        case 8:
            for (i = 0; i < 256; i++) {
                vga_set_palette_color(i, stdpalette8[i][0], stdpalette8[i][1],
                    stdpalette8[i][2]);
            }
            break;
    }
}

