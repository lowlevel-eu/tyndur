/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Alexander Siol.
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
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#ifndef __VESA_DRAWING_H__
#define __VESA_DRAWING_H__

#include <cdi/video.h>

void vesa_set_raster_op(struct cdi_video_device* device,
    enum cdi_video_raster_op rop);
void vesa_draw_line(struct cdi_video_bitmap* target, unsigned int x1,
    unsigned int y1, unsigned int x2, unsigned int y2,
    struct cdi_video_color* color);
void vesa_draw_rectangle(struct cdi_video_bitmap* target, unsigned int x,
    unsigned int y, unsigned int width, unsigned int height,
    struct cdi_video_color* color);
void vesa_draw_ellipse(struct cdi_video_bitmap* target, unsigned int x,
    unsigned int y, unsigned int width, unsigned int height,
    struct cdi_video_color* color);
void vesa_draw_bitmap(struct cdi_video_bitmap* target,
    struct cdi_video_bitmap* bitmap, unsigned int x, unsigned int y);
void vesa_draw_bitmap_part(struct cdi_video_bitmap* target,
    struct cdi_video_bitmap* bitmap, unsigned int x, unsigned int y,
    unsigned int srcx, unsigned int srcy, unsigned int width,
    unsigned int height);
void vesa_draw_dot(struct cdi_video_bitmap* target, unsigned int x,
    unsigned int y, struct cdi_video_color* color);

uint32_t do_rop(enum cdi_video_raster_op rop, uint32_t new, uint32_t old);
uint32_t convert_color(struct cdi_video_pixel_format fmt,
    struct cdi_video_color* cdicolor);
struct cdi_video_color* convert_color_from_pixel(struct cdi_video_pixel_format
    fmt, uint32_t value);

void* get_read_buffer_addr(struct cdi_video_bitmap* bitmap);
void* get_write_buffer_addr(struct cdi_video_bitmap* bitmap);

#endif /* __VESA_DRAWING_H__ */
