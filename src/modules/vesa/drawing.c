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
 
#include "stdlib.h"

#include "drawing.h"
#include "vbe.h"

int bitmap_count = 0;
int **bitmap_data = 0;
int *bitmap_width = 0;
int *bitmap_height= 0;
int *bitmap_handle = 0;

void clear_screen(int color)
{
    //if (!backbuffer) {
    //    return;
    //}
    draw_rect(0, 0, screen_xres, screen_yres, color);
}
void draw_dot(int x, int y, int color)
{
    if (!backbuffer) {
        return;
    }
    backbuffer[x + y * screen_xres] = color;
}
void draw_rect(int x, int y, int width, int height, int color)
{
    if (!backbuffer) {
        return;
    }
    int ix;
    int iy;
    for (ix = x; ix < x + width; ix++) {
        for (iy = y; iy < y + height; iy++) {
            backbuffer[ix + iy * screen_xres] = color;
        }
    }
    /*for (iy = y; iy < y + height; iy++) {
        memset(backbuffer + x + iy * screen_xres, color
    }*/
}
void draw_bitmap(int bitmap, int x, int y, int flags)
{
    if (!backbuffer) {
        return;
    }
    if (!bitmap) {
        return;
    }
    int i;
    for (i = 0; i < bitmap_count; i++) {
        if (bitmap_handle[i] == bitmap) {
            //Bitmap gefunden!
            int ix;
            int iy;
            for (ix = 0; ix < bitmap_width[i]; ix++) {
                for (iy = 0; iy < bitmap_height[i]; iy++) {
                    if (flags & 0x01) {
                        dword bgcolor = backbuffer[ix + x + (iy + y) * screen_xres];
                        dword fgcolor = bitmap_data[i][ix + iy * bitmap_width[i]];
                        byte transparency = (bitmap_data[i][ix + iy * bitmap_width[i]]
                            & 0xFF000000) >> 24;
                        float fgalpha = (float)transparency / 255;
                        float bgalpha = 1 - fgalpha;
                        backbuffer[ix + x + (iy + y) * screen_xres] =
                                + (int)((float)(bgcolor & 0xFF) * bgalpha)
                                + (int)((float)(fgcolor & 0xFF) * fgalpha)
                                + ((int)((float)((bgcolor  & 0xFF00) >> 8) * bgalpha) << 8)
                                + ((int)((float)((fgcolor & 0xFF00) >> 8) * fgalpha) << 8)
                                + ((int)((float)((bgcolor  & 0xFF0000) >> 16) * bgalpha) << 16)
                                + ((int)((float)((fgcolor & 0xFF0000) >> 16) * fgalpha) << 16);
                    } else {
                        backbuffer[ix + x + (iy + y) * screen_xres] =
                            bitmap_data[i][ix + iy * bitmap_width[i]];
                    }
                }
            }
            return;
        }
    }
}

int add_bitmap_to_list(int width, int height, int *data)
{
    int **tmp_bitmap_data = malloc(sizeof(int *) * (bitmap_count + 1));
    int *tmp_bitmap_handle = malloc(sizeof(int) * (bitmap_count + 1));
    int *tmp_bitmap_width = malloc(sizeof(int) * (bitmap_count + 1));
    int *tmp_bitmap_height = malloc(sizeof(int) * (bitmap_count + 1));
    memcpy(tmp_bitmap_data, bitmap_data, bitmap_count * sizeof(int *));
    memcpy(tmp_bitmap_handle, bitmap_handle, bitmap_count * sizeof(int));
    memcpy(tmp_bitmap_width, bitmap_width, bitmap_count * sizeof(int));
    memcpy(tmp_bitmap_height, bitmap_height, bitmap_count * sizeof(int));
    tmp_bitmap_data[bitmap_count] = data;
    tmp_bitmap_width[bitmap_count] = width;
    tmp_bitmap_height[bitmap_count] = height;
    if (bitmap_count > 0) {
        tmp_bitmap_handle[bitmap_count] = tmp_bitmap_handle[bitmap_count - 1] + 1;
        free(bitmap_handle);
        free(bitmap_width);
        free(bitmap_height);
        free(bitmap_data);
    } else {
        tmp_bitmap_handle[bitmap_count] = 1;
    }
    bitmap_handle = tmp_bitmap_handle;
    bitmap_data = tmp_bitmap_data;
    bitmap_width = tmp_bitmap_width;
    bitmap_height = tmp_bitmap_height;
    bitmap_count++;
    return bitmap_handle[bitmap_count - 1];
}
int delete_bitmap_from_list(int index)
{
    if (index <= 0) {
        return 0;
    }
    if (!bitmap_count) {
        return 0;
    }
    if (index >= bitmap_count) {
        return 0;
    }
    free(bitmap_data[index]);
    if (bitmap_count == 1) {
        free(bitmap_handle);
        free(bitmap_width);
        free(bitmap_height);
        free(bitmap_data);
        bitmap_handle = 0;
        bitmap_data = 0;
        bitmap_width = 0;
        bitmap_height = 0;
        bitmap_count = 0;
    } else {
        int **tmp_bitmap_data = malloc(sizeof(int *) * (bitmap_count - 1));
        int *tmp_bitmap_handle = malloc(sizeof(int) * (bitmap_count - 1));
        int *tmp_bitmap_width = malloc(sizeof(int) * (bitmap_count - 1));
        int *tmp_bitmap_height = malloc(sizeof(int) * (bitmap_count - 1));
        memcpy(tmp_bitmap_data, bitmap_data, index * sizeof(int *));
        memcpy(tmp_bitmap_handle, bitmap_handle, index * sizeof(int));
        memcpy(tmp_bitmap_width, bitmap_width, index * sizeof(int));
        memcpy(tmp_bitmap_height, bitmap_height, index * sizeof(int));
        memcpy(tmp_bitmap_data + index, bitmap_data + index + 1,
            (bitmap_count - index - 1) * sizeof(int *));
        memcpy(tmp_bitmap_handle + index, bitmap_handle + index + 1,
            (bitmap_count - index - 1) * sizeof(int));
        memcpy(tmp_bitmap_width + index, bitmap_width + index + 1,
            (bitmap_count - index - 1) * sizeof(int));
        memcpy(tmp_bitmap_height + index, bitmap_height + index + 1,
            (bitmap_count - index - 1) * sizeof(int));
        free(bitmap_handle);
        free(bitmap_width);
        free(bitmap_height);
        free(bitmap_data);
        bitmap_handle = tmp_bitmap_handle;
        bitmap_data = tmp_bitmap_data;
        bitmap_width = tmp_bitmap_width;
        bitmap_height = tmp_bitmap_height;
        bitmap_count--;
    }
    return 1;
}

int create_bitmap(int width, int height, int *data)
{
    return add_bitmap_to_list(width, height, data);
}
int delete_bitmap(int bitmap)
{
    if (!bitmap) {
        return 0;
    }
    int i;
    for (i = 0; i < bitmap_count; i++) {
        if (bitmap_handle[i] == bitmap) {
            delete_bitmap_from_list(i);
            return 1;
        }
    }
    return 0;
}

int update_screen(void)
{
    if (!backbuffer) { 
        return 0;
    }
    if (!vidmem) {
        return 0;
    }
    memcpy(vidmem, backbuffer, screen_xres * screen_yres * screen_bpp / 8);
    return 1;
}
int update_partly(int x, int y, int width, int height)
{
    if (!backbuffer) {
        return 0;
    }
    if (!vidmem) {
        return 0;
    }
    int ix;
    int iy;
    for (ix = 0; ix < width; ix++) {
        for (iy = 0; iy < height; iy++) {
            vidmem[ix + x + (iy + y) * screen_xres] =
                backbuffer[ix + x + (iy + y) * screen_xres];
        }
    }
    return 1;
}
