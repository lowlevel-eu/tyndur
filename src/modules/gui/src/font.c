/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Antoine Kaufmann.
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
 
#include "font.h"

#include "string.h"
#include "stdlib.h"

glyph_t *get_glyph(font_t font, char character)
{
    int i;
    for (i = 0; i < font.glyphcount; i++)
    {
        if (font.glyphs[i].character == character) {
            return &font.glyphs[i];
        }
    }
    return 0;
}
dword get_string_length(font_t font, char *string)
{
    if (!string) return 0;
    int length = 0;
    int i;
    for (i = 0; i < strlen(string); i++) {
        glyph_t *glyph = get_glyph(font, string[i]);
        if (glyph) {
            length = length + glyph->width;
        }
    }
    return length;
}
bitmap_t render_string(font_t font, char *string)
{
    bitmap_t bitmap;
    bitmap.width = get_string_length(font, string);
    bitmap.height = font.height;
    bitmap.data = malloc(bitmap.width * bitmap.height * sizeof(dword));
    if (!bitmap.data) {
        bitmap.width = 0;
        bitmap.height = 0;
        return bitmap;
    }
    memset(bitmap.data, 0xFF, bitmap.width * bitmap.height * sizeof(dword));
    int x = 0;
    int i;
    int ix;
    int iy;
    for (i = 0; i < strlen(string); i++) {
        glyph_t *glyph = get_glyph(font, string[i]);
        if (glyph) {
            //printf("%02X%02X%02X%02X\n", glyph->data[0], glyph->data[1], glyph->data[2], glyph->data[3]);
            //Zeichne Zeichen
            dword linesize = glyph->width;
            if (linesize & 0x7) linesize = (linesize & 0xFFFFFFF8) + 0x8;
            for (iy = 0; iy < (glyph->ascent + glyph->descent); iy++) {
                for (ix = 0; ix < glyph->width; ix++) {
                    if ((glyph->data[(ix >> 3) + iy * linesize / 8] >> (7 - (ix % 8))) & 1) {
                        bitmap.data[x + ix + (iy + font.max_ascent - glyph->ascent) * bitmap.width] = 0xFF000000;
                    } else {
                        bitmap.data[x + ix + (iy + font.max_ascent - glyph->ascent) * bitmap.width] = 0x00FFFFFF;
                    }
                    //printf("y: %d\n", iy * linesize / 8);
                }
            }
            
            x = x + glyph->width;
        }
    }
    return bitmap;
}
