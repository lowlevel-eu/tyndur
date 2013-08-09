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

#include <video/video.h>
#include <stdio.h>
#include <init.h>
#include <rpc.h>
#include <stdlib.h>
#include <video/commands.h>
#include <syscall.h>

extern driver_context_t* context;

void libvideo_draw_pixel(int x, int y)
{
    rpc_data_cmd_draw_pixel_t* cmd = NULL;

    if (context->cmdbufferpos + sizeof(*cmd) >= context->cmdbufferlen) {
        libvideo_do_command_buffer();
    }

    cmd = (rpc_data_cmd_draw_pixel_t*)(context->cmdbuffer +
        context->cmdbufferpos);
    *cmd = (rpc_data_cmd_draw_pixel_t) {
        .header = {
            .command = VIDEO_CMD_DRAW_PIXEL,
        },
        .x = x,
        .y = y,
    };

    context->cmdbufferpos += sizeof(*cmd);
}

void libvideo_draw_rectangle(int x, int y, int width, int height)
{
    rpc_data_cmd_draw_rectangle_t* cmd = NULL;

    if (context->cmdbufferpos + sizeof(*cmd) >= context->cmdbufferlen) {
        libvideo_do_command_buffer();
    }

    cmd = (rpc_data_cmd_draw_rectangle_t*)(context->cmdbuffer +
        context->cmdbufferpos);
    *cmd = (rpc_data_cmd_draw_rectangle_t) {
        .header = {
            .command = VIDEO_CMD_DRAW_RECTANGLE,
        },
        .x = x,
        .y = y,
        .width = width,
        .height = height,
    };

    context->cmdbufferpos += sizeof(*cmd);
}

void libvideo_draw_ellipse(int x, int y, int width, int height)
{
    rpc_data_cmd_draw_ellipse_t* cmd = NULL;

    if (context->cmdbufferpos + sizeof(*cmd) >= context->cmdbufferlen) {
        libvideo_do_command_buffer();
    }

    cmd = (rpc_data_cmd_draw_ellipse_t*)(context->cmdbuffer +
        context->cmdbufferpos);
    *cmd = (rpc_data_cmd_draw_ellipse_t) {
        .header = {
            .command = VIDEO_CMD_DRAW_ELLIPSE,
        },
        .x = x,
        .y = y,
        .width = width,
        .height = height,
    };

    context->cmdbufferpos += sizeof(*cmd);
}

void libvideo_draw_line(int x1, int y1, int x2, int y2)
{
    rpc_data_cmd_draw_line_t* cmd = NULL;

    if (context->cmdbufferpos + sizeof(*cmd) >= context->cmdbufferlen) {
        libvideo_do_command_buffer();
    }

    cmd = (rpc_data_cmd_draw_line_t*)(context->cmdbuffer +
        context->cmdbufferpos);
    *cmd = (rpc_data_cmd_draw_line_t) {
        .header = {
            .command = VIDEO_CMD_DRAW_LINE,
        },
        .x1 = x1,
        .y1 = y1,
        .x2 = x2,
        .y2 = y2,
    };

    context->cmdbufferpos += sizeof(*cmd);
}

void libvideo_draw_bitmap(video_bitmap_t *bitmap, int x, int y)
{
    rpc_data_cmd_draw_bitmap_t* cmd = NULL;

    if (context->cmdbufferpos + sizeof(*cmd) >= context->cmdbufferlen) {
        libvideo_do_command_buffer();
    }

    cmd = (rpc_data_cmd_draw_bitmap_t*)(context->cmdbuffer +
        context->cmdbufferpos);
    *cmd = (rpc_data_cmd_draw_bitmap_t) {
        .header = {
            .command = VIDEO_CMD_DRAW_BITMAP,
        },
        .dst_x = x,
        .dst_y = y,
        .src_bitmap_id = bitmap->id,
    };

    context->cmdbufferpos += sizeof(*cmd);
}

void libvideo_draw_bitmap_part(video_bitmap_t *bitmap, int x, int y, int srcx,
    int srcy, int width, int height)
{
    rpc_data_cmd_draw_bitmap_part_t* cmd = NULL;

    if (context->cmdbufferpos + sizeof(*cmd) >= context->cmdbufferlen) {
        libvideo_do_command_buffer();
    }

    cmd = (rpc_data_cmd_draw_bitmap_part_t*)(context->cmdbuffer +
        context->cmdbufferpos);
    *cmd = (rpc_data_cmd_draw_bitmap_part_t) {
        .header = {
            .command = VIDEO_CMD_DRAW_BITMAP_PART,
        },
        .dst_x         = x,
        .dst_y         = y,
        .src_bitmap_id = bitmap->id,
        .src_x         = srcx,
        .src_y         = srcy,
        .width         = width,
        .height        = height,
    };

    context->cmdbufferpos += sizeof(*cmd);
}
