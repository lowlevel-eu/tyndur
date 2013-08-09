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

#ifndef __VESA_BITMAP_H__
#define __VESA_BITMAP_H__

#include <cdi/video.h>

void vesa_bitmap_set_usage_hint(struct cdi_video_device *device,
    struct cdi_video_bitmap *bitmap, enum cdi_video_bitmap_usage_hint hint);
void vesa_bitmap_destroy(struct cdi_video_bitmap *bitmap);
struct cdi_video_bitmap *vesa_bitmap_create(struct cdi_video_device *device,
    unsigned int width, unsigned int height, void *data);
void vesa_bitmap_convert(struct cdi_video_bitmap* bitmap,
    struct cdi_video_pixel_format newfmt);
void vesa_bitmap_flip_buffers(struct cdi_video_bitmap* bitmap);
int vesa_bitmap_request_backbuffers(struct cdi_video_bitmap* bitmap,
    int backbuffercount);

#endif /* __VESA_BITMAP_H__ */
