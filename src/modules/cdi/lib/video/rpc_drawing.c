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

#include "video.h"
#include "context.h"
#include "bitmap.h"
#include <stdio.h>
#include <syscall.h>
#include <video/commands.h>
#include <video/error.h>

//#define NO_REAL_DRAWING

static inline int check_environment(driver_context_t* context)
{
    if (context == NULL) {
        return -EVID_NOCONTEXT;
    }
    else if (context->target == NULL) {
        return -EVID_NOTARGET;
    }
    return -EVID_NONE;
}

int rpc_cmd_draw_pixel(pid_t pid, driver_context_t** context,
    uint32_t** response, void* data, uint32_t* response_size)
{
    #ifndef NO_REAL_DRAWING
    rpc_data_cmd_draw_pixel_t *rpc = data;

    **response = check_environment(*context);
    if (**response == -EVID_NONE) {
        videodriver->draw_dot((*context)->target, rpc->x, rpc->y,
            &(*context)->color);
        **response = -EVID_NONE;
    }
    #endif
    return sizeof(rpc_data_cmd_draw_pixel_t);
}

int rpc_cmd_draw_rectangle(pid_t pid, driver_context_t** context,
    uint32_t** response, void* data, uint32_t* response_size)
{
    #ifndef NO_REAL_DRAWING
    rpc_data_cmd_draw_rectangle_t *rpc = data;

    **response = check_environment(*context);
    if (**response == 0) {
        videodriver->draw_rectangle((*context)->target, rpc->x, rpc->y,
            rpc->width, rpc->height, &(*context)->color);
        **response = 0;
    }
    #endif
    return sizeof(rpc_data_cmd_draw_rectangle_t);
}

int rpc_cmd_draw_line(pid_t pid, driver_context_t** context,
    uint32_t** response, void* data, uint32_t* response_size)
{
    #ifndef NO_REAL_DRAWING
    rpc_data_cmd_draw_line_t *rpc = data;

    **response = check_environment(*context);
    if (**response == 0) {
        videodriver->draw_line((*context)->target, rpc->x1, rpc->y1,
            rpc->x2, rpc->y2, &(*context)->color);
        **response = 0;
    }
    #endif
    return sizeof(rpc_data_cmd_draw_line_t);
}

int rpc_cmd_draw_ellipse(pid_t pid, driver_context_t** context,
    uint32_t** response, void* data, uint32_t* response_size)
{
    #ifndef NO_REAL_DRAWING
    rpc_data_cmd_draw_ellipse_t *rpc = data;

    **response = check_environment(*context);
    if (**response == 0) {
        videodriver->draw_ellipse((*context)->target, rpc->x, rpc->y,
            rpc->width, rpc->height, &(*context)->color);
        **response = 0;
    }
    #endif
    return sizeof(rpc_data_cmd_draw_ellipse_t);
}

int rpc_cmd_draw_bitmap_part(pid_t pid, driver_context_t** context,
    uint32_t** response, void* data, uint32_t* response_size)
{
    #ifndef NO_REAL_DRAWING
    rpc_data_cmd_draw_bitmap_part_t *rpc = data;

    **response = check_environment(*context);
    if (**response == 0) {
        interfacebitmap_t* bitmap;
        bitmap = bitmap_get_by_id(rpc->src_bitmap_id);
        // FIXME: Bitmap-Owner?
        if (bitmap) {
            videodriver->draw_bitmap_part((*context)->target,
                bitmap->bitmap, rpc->dst_x, rpc->dst_y, rpc->src_x, rpc->src_y,
                rpc->width, rpc->height);
            **response = 0;
        }
        else {
            **response = -EVID_FAILED;
        }
    }
    #endif
    return sizeof(rpc_data_cmd_draw_bitmap_part_t);
}

int rpc_cmd_draw_bitmap(pid_t pid, driver_context_t** context,
    uint32_t** response, void* data, uint32_t* response_size)
{
    #ifndef NO_REAL_DRAWING
    rpc_data_cmd_draw_bitmap_t *rpc = data;

    **response = check_environment(*context);
    if (**response == 0) {
        interfacebitmap_t* if_bitmap;
        if_bitmap = bitmap_get_by_id(rpc->src_bitmap_id);
        // FIXME: Bitmap-Owner?
        if (if_bitmap) {
            videodriver->draw_bitmap((*context)->target,
                if_bitmap->bitmap, rpc->dst_x, rpc->dst_y);
            **response = 0;
        }
        else {
            **response = -EVID_FAILED;
        }
    }
    #endif
    return sizeof(rpc_data_cmd_draw_bitmap_t);
}
