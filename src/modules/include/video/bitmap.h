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

#ifndef VIDEO_BITMAP_H
#define VIDEO_BITMAP_H

#include "types.h"
#include "collections.h"

typedef struct bitmap_t
{
	dword width;
	dword height;
	dword bpp;
	byte *data;
} bitmap_t;

typedef struct sprite_t
{
	dword bpp;
	dword span_count;
	sword *span_data;
	dword pixel_count;
	byte *pixel_data;
} sprite_t;

void draw_dot_4p(bitmap_t bitmap, int x, int y, byte c);

void convert_bitmap_to_4(bitmap_t *bitmap);
void convert_bitmap(bitmap_t *bitmap, dword bpp);

sprite_t convert_bitmap_to_sprite(bitmap_t bitmap, dword transparent);

void bitmap_clear(bitmap_t bitmap, dword color);
void bitmap_draw_rect(bitmap_t bitmap, dword color, int x, int y, dword width, dword height);
void bitmap_draw_bitmap(bitmap_t bitmap, bitmap_t bitmap2, int x, int y, dword width, dword height);
void bitmap_draw_sprite(bitmap_t bitmap, sprite_t sprite, int x, int y);

#endif
