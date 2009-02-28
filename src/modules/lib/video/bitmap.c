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
#include "video/color.h"

#include "stdlib.h"

void copy_shifted(byte *dest, byte *src, byte shift, dword length)
{
    //TODO: Kann weiter optimiert werden (größere Datentypen)
    int i;
    byte mask1 = 0xFF >> shift;
    byte mask2 = ~mask1;
    for (i = 0; i < length; i++) {
        dest[i] &= ~mask1;
        dest[i] |= src[i] >> shift;
        dest[i + 1] &= ~mask2;
        dest[i + 1] |= src[i] << (8 - shift);
    }
}

void draw_dot_4p(bitmap_t bitmap, int x, int y, byte c)
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
byte get_color_4p(bitmap_t bitmap, int x, int y)
{
    unsigned wd_in_bytes, off, mask, pagesize;
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

inline dword get_color(bitmap_t bitmap, int x, int y)
{
    switch (bitmap.bpp)
    {
        case 1:
            return 0;
        case 2:
            return 0;
        case 4:
            return convert_color_4_to_24(get_color_4p(bitmap, x, y));
        case 8:
            return 0;
        case 16:
            return 0;
        case 24:
            return 0;
        case 32:
            return ((dword*)bitmap.data)[x + y * bitmap.width];
        default:
            return 0;
    }
}

void convert_bitmap_to_4(bitmap_t *bitmap)
{
    dword *bitmapdata;
    switch (bitmap->bpp)
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
            bitmapdata = (dword*)bitmap->data;
            bitmap->bpp = 4;
            bitmap->data = malloc(bitmap->width * bitmap->height / 2);
            int ix, iy;
            for (iy = 0; iy < bitmap->height; iy++) {
                for (ix = 0; ix < bitmap->width; ix++) {
                    draw_dot_4p(*bitmap, ix, iy, convert_color_24_to_4(bitmapdata[ix + iy * bitmap->width]));
                }
            }
            free(bitmapdata);
            break;
    }
}
void convert_bitmap(bitmap_t *bitmap, dword bpp)
{
    switch (bpp)
    {
        case 1:
            break;
        case 2:
            break;
        case 4:
            convert_bitmap_to_4(bitmap);
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

//Quelle: http://www.exaflop.org/docs/fastsprites/
sprite_t convert_bitmap_to_sprite(bitmap_t bitmap, dword transparentcolor)
{
    sprite_t sprite;
    sprite.bpp = bitmap.bpp;
    //Segment-Daten schreiben
    //TODO: Dauerndes realloc() ist recht ineffektiv...
    sprite.span_count = 1;
    sprite.span_data = malloc(2);
    int transparent = 0;
    int length = 0;
    if (get_color(bitmap, 0, 0) == transparentcolor) {
        transparent = 1;
    }
    int i;
    for (i = 0; i < bitmap.width * bitmap.height; i++) {
        int is_transparent = (get_color(bitmap, i % bitmap.width, i / bitmap.width) == transparentcolor);
        if (is_transparent != transparent) {
            //Länge/Status speichern
            if (transparent) {
                sprite.span_data[sprite.span_count - 1] = -length;
            } else {
                sprite.span_data[sprite.span_count - 1] = length;
            }
            sprite.span_count++;
            sprite.span_data = realloc(sprite.span_data, sprite.span_count * 2);
            //Neues Segment anfangen
            length = 1;
            transparent = is_transparent;
        } else {
            length++;
        }
        if ((i % bitmap.width) == (bitmap.width - 1)) {
            //Nächste Zeile
            if (transparent) {
                sprite.span_data[sprite.span_count - 1] = -length;
            } else {
                sprite.span_data[sprite.span_count - 1] = length;
            }
            sprite.span_count++;
            sprite.span_data = realloc(sprite.span_data, sprite.span_count * 2);
            sprite.span_data[sprite.span_count - 1] = 0;
            sprite.span_count++;
            sprite.span_data = realloc(sprite.span_data, sprite.span_count * 2);
            length = 0;
            if ((i / bitmap.width + 1) < (bitmap.height)) {
                transparent = (get_color(bitmap, 0, i / bitmap.width + 1) == transparentcolor);
            }
        }
    }
    //Segment-Daten beenden
    sprite.span_data[sprite.span_count - 1] = 0;
    //Farbdaten schreiben
    sprite.pixel_count = 0;
    sprite.pixel_data = 0;
    int position = 0;
    int pixel_width = bitmap.bpp / 8;
    if (!pixel_width) {
        pixel_width = 1;
    }
    for (i = 0; i < sprite.span_count; i++) {
        if (sprite.span_data[i] > 0) {
            sprite.pixel_count+= sprite.span_data[i];
            sprite.pixel_data = realloc(sprite.pixel_data, sprite.pixel_count * pixel_width);
            int j;
            for (j = 0; j < sprite.span_data[i]; j++) {
                //printf("Neuer Pixel: %08X\n", bitmapdata[position + j]);
                //((dword*)sprite.pixel_data)[sprite.pixel_count - sprite.span_data[i] + j] = bitmapdata[position + j];
                switch (bitmap.bpp)
                {
                    case 1:
                        break;
                    case 2:
                        break;
                    case 4:
                        sprite.pixel_data[sprite.pixel_count - sprite.span_data[i] + j]
                            = get_color_4p(bitmap, (position + j) % bitmap.width, (position + j) / bitmap.width);
                        break;
                    case 8:
                        break;
                    case 16:
                        break;
                    case 24:
                        break;
                    case 32:
                        ((dword*)sprite.pixel_data)[sprite.pixel_count - sprite.span_data[i] + j]
                            = get_color(bitmap, (position + j) % bitmap.width, (position + j) / bitmap.width);
                        break;
                }
            }
            position += sprite.span_data[i];
        } else {
            position -= sprite.span_data[i];
        }
    }
    return sprite;
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
            color8 = convert_color_24_to_4(color);
            bitmapdata8 = bitmap.data;
            if ((x >> 3) < ((x + width) >> 3)) {
                dword linesize = bitmap.width >> 3;
                if (bitmap.width & 0x7) {
                    linesize++;
                }
                int pagesize = bitmap.height * linesize;
                int iy;
                //Linken Streifen zeichnen
                if (x & 0x7) {
                    byte mask = 0xFF >> (x & 0x7);
                    for (iy = y; iy < (y + height); iy++) {
                        dword offset = linesize * iy + (x >> 3);
                        if (color8 & 1) {
                            bitmapdata8[offset] |= mask;
                        } else {
                            bitmapdata8[offset] &= ~mask;
                        }
                        if (color8 & 2) {
                            bitmapdata8[offset + pagesize] |= mask;
                        } else {
                            bitmapdata8[offset + pagesize] &= ~mask;
                        }
                        if (color8 & 4) {
                            bitmapdata8[offset + pagesize * 2] |= mask;
                        } else {
                            bitmapdata8[offset + pagesize * 2] &= ~mask;
                        }
                        if (color8 & 8) {
                            bitmapdata8[offset + pagesize * 3] |= mask;
                        } else {
                            bitmapdata8[offset + pagesize * 3] &= ~mask;
                        }
                    }
                }
                //Mittelteil zeichnen
                if (((int)width - (8 - (x & 0x7))) > 8) {
                    for (iy = y; iy < (y + height); iy++) {
                        dword offset;
                        if (x & 0x7) {
                            offset = linesize * iy + (x >> 3) + 1;
                        } else {
                            offset = linesize * iy + (x >> 3);
                        }
                        dword length;
                        if (x & 0x7) {
                            length = (width - (8 - (x & 0x7))) >> 3;
                        } else {
                            length = width >> 3;
                        }
                        if (color8 & 1) {
                            memset(bitmapdata8 + offset, 0xFF, length);
                        } else {
                            memset(bitmapdata8 + offset, 0, length);
                        }
                        if (color8 & 2) {
                            memset(bitmapdata8 + offset + pagesize, 0xFF, length);
                        } else {
                            memset(bitmapdata8 + offset + pagesize, 0, length);
                        }
                        if (color8 & 4) {
                            memset(bitmapdata8 + offset + pagesize * 2, 0xFF, length);
                        } else {
                            memset(bitmapdata8 + offset + pagesize * 2, 0, length);
                        }
                        if (color8 & 8) {
                            memset(bitmapdata8 + offset + pagesize * 3, 0xFF, length);
                        } else {
                            memset(bitmapdata8 + offset + pagesize * 3, 0, length);
                        }
                    }
                }
                //Rechten Streifen zeichnen
                if ((x + width) & 0x7) {
                    byte mask = 0xFF << (8 - ((x + width) & 0x7));
                    for (iy = y; iy < (y + height); iy++) {
                        if (color8 & 1) {
                            bitmapdata8[linesize * iy + ((x + width) >> 3)] |= mask;
                        } else {
                            bitmapdata8[linesize * iy + ((x + width) >> 3)] &= ~mask;
                        }
                        if (color8 & 2) {
                            bitmapdata8[linesize * iy + ((x + width) >> 3) + pagesize] |= mask;
                        } else {
                            bitmapdata8[linesize * iy + ((x + width) >> 3) + pagesize] &= ~mask;
                        }
                        if (color8 & 4) {
                            bitmapdata8[linesize * iy + ((x + width) >> 3) + pagesize * 2] |= mask;
                        } else {
                            bitmapdata8[linesize * iy + ((x + width) >> 3) + pagesize * 2] &= ~mask;
                        }
                        if (color8 & 8) {
                            bitmapdata8[linesize * iy + ((x + width) >> 3) + pagesize * 3] |= mask;
                        } else {
                            bitmapdata8[linesize * iy + ((x + width) >> 3) + pagesize * 3] &= ~mask;
                        }
                    }
                }
            } else {
                for (ix = x; ix < (x + width); ix++) {
                    for (iy = y; iy < (y + height); iy++) {
                        draw_dot_4p(bitmap, ix, iy, color8);
                    }
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
    int oldwidth = width;
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
    int iy;
    switch (bitmap.bpp)
    {
        case 1:
            break;
        case 2:
            break;
        case 4:
            if (((x & 0x7) == 0) && ((offsetx & 0x7) == 0) && ((width & 0x7) == 0)) {
                //Alle Maße passend, ein simples memcpy reicht.
                dword linesize = bitmap.width >> 3;
                if (bitmap.width & 0x7) {
                    linesize++;
                }
                dword linesize2 = bitmap2.width >> 3;
                if (bitmap2.width & 0x7) {
                    linesize2++;
                }
                int pagesize1 = bitmap.height * linesize;
                int pagesize2 = bitmap2.height * linesize2;
                for (iy = 0; iy < height; iy++) {
                    byte *data1 = bitmap.data + (y + offsety + iy) * (bitmap.width >> 3) + (x >> 3) + (offsetx >> 3);
                    byte *data2 = bitmap2.data + (iy + offsety) * (bitmap2.width >> 3) + (offsetx >> 3);
                    memcpy(data1, data2, width >> 3);
                    memcpy(data1 + pagesize1, data2 + pagesize2, width >> 3);
                    memcpy(data1 + pagesize1 * 2, data2 + pagesize2 * 2, width >> 3);
                    memcpy(data1 + pagesize1 * 3, data2 + pagesize2 * 3, width >> 3);
                }
            } else {
                /*
                for (ix = 0; ix < width; ix++) {
                    for (iy = 0; iy < height; iy++) {
                        byte color = get_color_4p(bitmap2, ix + offsetx, offsety + iy);
                        draw_dot_4p(bitmap, x + offsetx + ix, y + offsety + iy, color);
                    }
                }*/
                //Optimiert:
                dword linesize = bitmap.width >> 3;
                if (bitmap.width & 0x7) {
                    linesize++;
                }
                dword linesize2 = bitmap2.width >> 3;
                if (bitmap2.width & 0x7) {
                    linesize2++;
                }
                int pagesize1 = bitmap.height * linesize;
                int pagesize2 = bitmap2.height * linesize2;
                if (((x & 0x7) == 0) && ((offsetx & 0x7) == 0)) {
                    //Ohne Bitshift kopieren
                    for (iy = 0; iy < height; iy++) {
                        //Hauptteil kopieren
                        dword srcoffset = offsetx >> 3;
                        byte *data1 = bitmap.data + linesize * (iy + y + offsety) + ((offsetx + x) >> 3);
                        byte *data2 = bitmap2.data + linesize2 * (iy + offsety) + srcoffset;
                        memcpy(data1, data2, width >> 3);
                        memcpy(data1 + pagesize1, data2 + pagesize2, width >> 3);
                        memcpy(data1 + pagesize1 * 2, data2 + pagesize2 * 2, width >> 3);
                        memcpy(data1 + pagesize1 * 3, data2 + pagesize2 * 3, width >> 3);
                        //Rechten Streifen kopieren
                        byte mask = 0xFF << (8 - (width & 0x7));
                        dword offset1 = linesize * (iy + y + offsety) + ((offsetx + x) >> 3) + (width >> 3);
                        dword offset2 = linesize2 * (iy + offsety) + srcoffset + (width >> 3);
                        byte data = bitmap2.data[offset2];
                        bitmap.data[offset1] &= ~mask;
                        bitmap.data[offset1] |= (data & mask);
                        data = bitmap2.data[offset2 + pagesize2];
                        bitmap.data[offset1 + pagesize1] &= ~mask;
                        bitmap.data[offset1 + pagesize1] |= (data & mask);
                        data = bitmap2.data[offset2 + pagesize2 * 2];
                        bitmap.data[offset1 + pagesize1 * 2] &= ~mask;
                        bitmap.data[offset1 + pagesize1 * 2] |= (data & mask);
                        data = bitmap2.data[offset2 + pagesize2 * 3];
                        bitmap.data[offset1 + pagesize1 * 3] &= ~mask;
                        bitmap.data[offset1 + pagesize1 * 3] |= (data & mask);
                    }
                } else {
                    //Verschoben kopieren
                    if (x < 0) {
                        for (iy = 0; iy < height; iy++) {
                            if ((((oldwidth & ~0x7) - offsetx) >> 3) > -1) {
                                //Linken Streifen kopieren
                                dword srcoffset = offsetx >> 3;
                                byte shift = 8 - (((dword)-x) & 0x7);
                                byte mask = 0xFF << (8 - shift);
                                byte data = bitmap2.data[linesize2 * (iy + offsety) + srcoffset];
                                bitmap.data[linesize * (iy + y + offsety)] &= ~mask;
                                bitmap.data[linesize * (iy + y + offsety)] |= data << (8 - shift);
                                data = bitmap2.data[linesize2 * (iy + offsety) + srcoffset + pagesize2];
                                bitmap.data[linesize * (iy + y + offsety) + pagesize1] &= ~mask;
                                bitmap.data[linesize * (iy + y + offsety) + pagesize1] |= data << (8 - shift);
                                data = bitmap2.data[linesize2 * (iy + offsety) + srcoffset + pagesize2 * 2];
                                bitmap.data[linesize * (iy + y + offsety) + pagesize1 * 2] &= ~mask;
                                bitmap.data[linesize * (iy + y + offsety) + pagesize1 * 2] |= data << (8 - shift);
                                data = bitmap2.data[linesize2 * (iy + offsety) + srcoffset + pagesize2 * 3];
                                bitmap.data[linesize * (iy + y + offsety) + pagesize1 * 3] &= ~mask;
                                bitmap.data[linesize * (iy + y + offsety) + pagesize1 * 3] |= data << (8 - shift);
                                //Hauptteil kopieren
                                if ((((oldwidth & ~0x7) - offsetx) >> 3) > 0) {
                                    dword length = (((oldwidth & ~0x7) - offsetx) >> 3);
                                    byte *data1 = bitmap.data + linesize * (iy + y + offsety);
                                    byte *data2 = bitmap2.data + linesize2 * (iy + offsety) + srcoffset + 1;
                                    copy_shifted(data1, data2, shift, length);
                                    copy_shifted(data1 + pagesize1, data2 + pagesize2, shift, length);
                                    copy_shifted(data1 + pagesize1 * 2, data2 + pagesize2 * 2, shift, length);
                                    copy_shifted(data1 + pagesize1 * 3, data2 + pagesize2 * 3, shift, length);
                                }
                            }
                        }
                    } else {
                        for (iy = 0; iy < height; iy++) {
                            //Hauptteil kopieren
                            byte *data1 = bitmap.data + (x >> 3) + linesize * (iy + y + offsety);
                            byte *data2 = bitmap2.data + linesize2 * (iy + offsety);
                            copy_shifted(data1, data2, x & 0x7, width >> 3);
                            copy_shifted(data1 + pagesize1, data2 + pagesize2, x & 0x7, width >> 3);
                            copy_shifted(data1 + pagesize1 * 2, data2 + pagesize2 * 2, x & 0x7, width >> 3);
                            copy_shifted(data1 + pagesize1 * 3, data2 + pagesize2 * 3, x & 0x7, width >> 3);
                        }
                    }
                    int rightedge = width >> 3;
                    if (x < 0) {
                        width += offsetx;
                        rightedge = (oldwidth >> 3) - (offsetx >> 3) - 1;
                    } else {
                        rightedge += x >> 3;
                    }
                    for (iy = 0; iy < height; iy++) {
                        //Rechten Streifen kopieren
                        if (width & 0x7) {
                            byte mask = 0xFF << (8 - (width & 0x7));
                            byte data = bitmap2.data[linesize2 * (iy + offsety) + (width >> 3)];
                            data &= mask;
                            dword offset1 = linesize * (iy + y + offsety) + rightedge;
                            dword offset2 = linesize * (iy + y + offsety) + rightedge + 1;
                            if (rightedge >= 0) {
                                bitmap.data[offset1] &= ~(mask >> (x & 0x7));
                                bitmap.data[offset1] |= data >> (x & 0x7);
                            }
                            if (((width + x) >> 3) < linesize2) {
                                bitmap.data[offset2] &= ~(mask << (8 - (x & 0x7)));
                                bitmap.data[offset2] |= data << (8 - (x & 0x7));
                            }
                            data = bitmap2.data[linesize2 * (iy + offsety) + (width >> 3) + pagesize2];
                            data &= mask;
                            if (rightedge >= 0) {
                                bitmap.data[offset1 + pagesize1] &= ~(mask >> (x & 0x7));
                                bitmap.data[offset1 + pagesize1] |= data >> (x & 0x7);
                            }
                            if (((width + x) >> 3) < linesize2) {
                                bitmap.data[offset2 + pagesize1] &= ~(mask << (8 - (x & 0x7)));
                                bitmap.data[offset2 + pagesize1] |= data << (8 - (x & 0x7));
                            }
                            data = bitmap2.data[linesize2 * (iy + offsety) + (width >> 3) + pagesize2 * 2];
                            data &= mask;
                            if (rightedge >= 0) {
                                bitmap.data[offset1 + pagesize1 * 2] &= ~(mask >> (x & 0x7));
                                bitmap.data[offset1 + pagesize1 * 2] |= data >> (x & 0x7);
                            }
                            if (((width + x) >> 3) < linesize2) {
                                bitmap.data[offset2 + pagesize1 * 2] &= ~(mask << (8 - (x & 0x7)));
                                bitmap.data[offset2 + pagesize1 * 2] |= data << (8 - (x & 0x7));
                            }
                            data = bitmap2.data[linesize2 * (iy + offsety) + (width >> 3) + pagesize2 * 3];
                            data &= mask;
                            if (rightedge >= 0) {
                                bitmap.data[offset1 + pagesize1 * 3] &= ~(mask >> (x & 0x7));
                                bitmap.data[offset1 + pagesize1 * 3] |= data >> (x & 0x7);
                            }
                            if (((width + x) >> 3) < linesize2) {
                                bitmap.data[offset2 + pagesize1 * 3] &= ~(mask << (8 - (x & 0x7)));
                                bitmap.data[offset2 + pagesize1 * 3] |= data << (8 - (x & 0x7));
                            }
                        }
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

void bitmap_draw_sprite(bitmap_t bitmap, sprite_t sprite, int x, int y)
{
    int position = 0;
    int ix = 0;
    int iy = 0;
    int i, j;
    switch (bitmap.bpp)
    {
        case 1:
            break;
        case 2:
            break;
        case 4:
            for (i = 0; i < sprite.span_count; i++) {
                if ((iy + y) >= bitmap.height) {
                    break;
                }
                if (sprite.span_data[i] > 0) {
                    for (j = 0; j < sprite.span_data[i]; j++) {
                        if ((ix + x + j) >= bitmap.width) {
                            break;
                        }
                        if (((ix + x + j) >= 0) && ((iy + y) >= 0)) {
                            draw_dot_4p(bitmap, x + ix + j, y + iy, sprite.pixel_data[position + j]);
                        }
                    }
                    ix += sprite.span_data[i];
                    position += sprite.span_data[i];
                } else if (sprite.span_data[i] < 0) {
                    ix -= sprite.span_data[i];
                } else {
                    iy++;
                    ix = 0;
                }
            }
            break;
        case 8:
            break;
        case 16:
            break;
        case 24:
            break;
        case 32:
            for (i = 0; i < sprite.span_count; i++) {
                if ((iy + y) >= bitmap.height) {
                    break;
                }
                dword *bitmapdata = (dword*)bitmap.data;
                dword *pixeldata = (dword*)sprite.pixel_data;
                dword offset = bitmap.width * (y + iy) + x;
                if (sprite.span_data[i] > 0) {
                    for (j = 0; j < sprite.span_data[i]; j++) {
                        if ((ix + x + j) >= bitmap.width) {
                            break;
                        }
                        if (((ix + x + j) >= 0) && ((iy + y) >= 0)) {
                            bitmapdata[offset + ix + j] = pixeldata[position + j];
                        }
                    }
                    ix += sprite.span_data[i];
                    position += sprite.span_data[i];
                } else if (sprite.span_data[i] < 0) {
                    ix -= sprite.span_data[i];
                } else {
                    iy++;
                    ix = 0;
                }
            }
            break;
    }
}
