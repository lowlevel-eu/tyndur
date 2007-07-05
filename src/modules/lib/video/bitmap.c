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

#include "video/bitmap.h"

#include "stdlib.h"

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

inline void draw_dot_4p(bitmap_t bitmap, int x, int y, byte c)
{
    unsigned wd_in_bytes, off, mask, p, pmask, pagesize;
    
    wd_in_bytes = bitmap.width / 8;
    pagesize = bitmap.height * wd_in_bytes;
    off = wd_in_bytes * y + x / 8;
    x = (x & 7) * 1;
    mask = 0x80 >> x;
    pmask = 1;
    for(p = 0; p < 4; p++)
    {
        if(pmask & c)
            bitmap.data[off] = bitmap.data[off] | mask;
        else
            bitmap.data[off] = bitmap.data[off] & ~mask;
        pmask <<= 1;
        off += pagesize;
    }
}
inline byte get_color_4p(bitmap_t bitmap, int x, int y)
{
    unsigned wd_in_bytes, off, mask, p, pmask, pagesize;
    wd_in_bytes = bitmap.width / 8;
    pagesize = bitmap.height * wd_in_bytes;
    off = wd_in_bytes * y + x / 8;
    x = (x & 7) * 1;
    mask = 0x80 >> x;
    
    byte color = ((bitmap.data[off] & mask) >> (7 - x));
    color |= ((bitmap.data[off + pagesize] & mask) >> (7 - x)) << 1;
    color |= ((bitmap.data[off + pagesize * 2] & mask) >> (7 - x)) << 2;
    color |= ((bitmap.data[off + pagesize * 3] & mask) >> (7 - x)) << 3;
    return color;
}

void bitmap_clear(bitmap_t bitmap, dword color)
{
    switch (bitmap.bpp)
    {
        case 1:
            break;
        case 2:
            break;
        case 4:
            break;
        case 8:
            break;
        case 16:
            break;
        case 24:
            break;
        case 32:
            break;
    }
}
void bitmap_draw_rect(bitmap_t bitmap, dword color, int x, int y, dword width, dword height)
{
    //Maße anpassen
    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    if ((x + width) > bitmap.width) {
        width = bitmap.width - x;
    }
    if ((y + height) > bitmap.height) {
        height = bitmap.height - y;
    }

    //Je nach bpp verschieden zeichnen
    int ix, iy;
    dword *bitmapdata32;
    byte *bitmapdata8;
    byte color8;
    switch (bitmap.bpp)
    {
        case 1:
            //TODO
            break;
        case 2:
            //TODO
            break;
        case 4:
            //TODO
            color8 = convert_color_24_to_4(color);
            for (ix = x; ix < (x + width); ix++) {
                for (iy = y; iy < (y + height); iy++) {
                    draw_dot_4p(bitmap, ix, iy, color8);
                }
            }
            break;
        case 8:
            color8 = convert_color_24_to_8(color);
            bitmapdata8 = bitmap.data;
            for (iy = y; iy < (y + height); iy++) {
                for (ix = x; ix < (x + width); ix++) {
                    bitmapdata8[iy * bitmap.width + ix] = color8;
                }
            }
            break;
        case 16:
            //TODO
            break;
        case 24:
            //TODO
            break;
        case 32:
            bitmapdata32 = (dword*)bitmap.data;
            for (iy = y; iy < (y + height); iy++) {
                for (ix = x; ix < (x + width); ix++) {
                    bitmapdata32[iy * bitmap.width + ix] = color;
                }
            }
            break;
    }
}

void bitmap_draw_bitmap(bitmap_t bitmap, bitmap_t bitmap2, int x, int y, dword width, dword height)
{
    //printf("Zeichne Bitmap!\n");
    //Maße anpassen
    if (width == 0) {
        width = bitmap2.width;
    }
    if (height == 0) {
        height = bitmap2.height;
    }
    int offsetx = 0;
    int offsety = 0;
    if (x < 0) {
        offsetx = -x;
        width -= offsetx;
    }
    if (y < 0) {
        offsety = -y;
        height -= offsety;
    }
    if (x + offsetx + width > bitmap.width) {
        width = bitmap.width - x - offsetx;
    }
    if (y + offsety + height > bitmap.height) {
        height = bitmap.height - y - offsety;
    }
    //Bitmap zeichnen
    int ix, iy;
    switch (bitmap.bpp)
    {
        case 1:
            break;
        case 2:
            break;
        case 4:
            if (((x & 0x7) == 0) && ((offsetx & 0x7) == 0) && ((width & 0x7) == 0)) {
                for (iy = 0; iy < height; iy++) {
                    int pagesize1 = bitmap.height * (bitmap.width >> 3);
                    int pagesize2 = bitmap2.height * (bitmap2.width >> 3);
                    memcpy(bitmap.data + (y + offsety + iy) * (bitmap.width >> 3) + (x >> 3) + (offsetx >> 3),
                        bitmap2.data + (iy + offsety) * (bitmap2.width >> 3) + (offsetx >> 3), width >> 3);
                    memcpy(bitmap.data + (y + offsety + iy) * (bitmap.width >> 3) + (x >> 3) + (offsetx >> 3) + pagesize1,
                        bitmap2.data + (iy + offsety) * (bitmap2.width >> 3) + (offsetx >> 3) + pagesize2, width >> 3);
                    memcpy(bitmap.data + (y + offsety + iy) * (bitmap.width >> 3) + (x >> 3) + (offsetx >> 3) + pagesize1 * 2,
                        bitmap2.data + (iy + offsety) * (bitmap2.width >> 3) + (offsetx >> 3) + pagesize2 * 2, width >> 3);
                    memcpy(bitmap.data + (y + offsety + iy) * (bitmap.width >> 3) + (x >> 3) + (offsetx >> 3) + pagesize1 * 3,
                        bitmap2.data + (iy + offsety) * (bitmap2.width >> 3) + (offsetx >> 3) + pagesize2 * 3, width >> 3);
                }
            } else {
                //TODO: Optimieren!
                for (ix = 0; ix < width; ix++) {
                    for (iy = 0; iy < height; iy++) {
                        byte color = get_color_4p(bitmap2, ix + offsetx, offsety + iy);
                        draw_dot_4p(bitmap, x + offsetx + ix, y + offsety + iy, color);
                    }
                }
            }
            break;
        case 8:
            for (iy = 0; iy < height; iy++) {
                memcpy(bitmap.data + (y + offsety + iy) * bitmap.width + x + offsetx,
                    bitmap2.data + (iy + offsety) * bitmap2.width + offsetx, width);
            }
            break;
        case 16:
            break;
        case 24:
            break;
        case 32:
            for (iy = 0; iy < height; iy++) {
                dword *bitmapdata = (dword*)bitmap.data;
                dword *bitmapdata2 = (dword*)bitmap2.data;
                memcpy(bitmapdata + (y + offsety + iy) * bitmap.width + x + offsetx,
                    bitmapdata2 + (iy + offsety) * bitmap2.width + offsetx, width * 4);
            }
            break;
    }

}
