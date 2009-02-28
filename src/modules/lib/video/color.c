/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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

#include "video/bitmap.h"

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

byte convert_color_24_to_8(dword color)
{
    byte bestcolor = 0;

    int diffr = (byte)(((color & 0xFF0000) >> 16) - stdpalette8[0][0]);
    if (diffr < 0) diffr = -diffr;
    int diffg = (byte)(((color & 0xFF00) >> 8) - stdpalette8[0][1]);
    if (diffg < 0) diffg = -diffg;
    int diffb = (byte)((color & 0xFF) - stdpalette8[0][2]);
    if (diffb < 0) diffb = -diffb;
    dword bestdifference = diffr + diffg + diffb;
    int i;
    for (i = 1; i < 56; i++) {
        diffr = (byte)(((color & 0xFF0000) >> 16) - stdpalette8[i][0]);
        if (diffr < 0) diffr = -diffr;
        diffg = (byte)(((color & 0xFF00) >> 8) - stdpalette8[i][1]);
        if (diffg < 0) diffg = -diffg;
        diffb = (byte)((color & 0xFF) - stdpalette8[i][2]);
        if (diffb < 0) diffb = -diffb;
        dword difference = diffr + diffg + diffb;
        if (difference < bestdifference) {
            bestcolor = i;
            bestdifference = difference;
        }
    }
    return bestcolor;
}
byte convert_color_24_to_4(dword color)
{
    byte bestcolor = 0;

    int diffr = (byte)(((color & 0xFF0000) >> 16) - stdpalette4[0][0]);
    if (diffr < 0) diffr = -diffr;
    int diffg = (byte)(((color & 0xFF00) >> 8) - stdpalette4[0][1]);
    if (diffg < 0) diffg = -diffg;
    int diffb = (byte)((color & 0xFF) - stdpalette4[0][2]);
    if (diffb < 0) diffb = -diffb;
    dword bestdifference = diffr + diffg + diffb;
    int i;
    for (i = 1; i < 16; i++) {
        diffr = (byte)(((color & 0xFF0000) >> 16) - stdpalette4[i][0]);
        if (diffr < 0) diffr = -diffr;
        diffg = (byte)(((color & 0xFF00) >> 8) - stdpalette4[i][1]);
        if (diffg < 0) diffg = -diffg;
        diffb = (byte)((color & 0xFF) - stdpalette4[i][2]);
        if (diffb < 0) diffb = -diffb;
        dword difference = diffr + diffg + diffb;
        if (difference < bestdifference) {
            bestcolor = i;
            bestdifference = difference;
        }
    }
    return bestcolor;
}

dword convert_color_8_to_24(byte color)
{
    return (stdpalette8[color][0] << 16) + (stdpalette8[color][1] << 8) + stdpalette8[color][2];
}
dword convert_color_4_to_24(byte color)
{
    return (stdpalette4[color][0] << 16) + (stdpalette4[color][1] << 8) + stdpalette4[color][2];
}

