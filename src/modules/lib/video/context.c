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

int libvideo_change_device(int devicenum)
{
    rpc_data_cmd_use_device_t* cmd = NULL;

    if (context->cmdbufferpos + sizeof(*cmd) >= context->cmdbufferlen) {
        libvideo_do_command_buffer();
    }

    cmd = (rpc_data_cmd_use_device_t*)(context->cmdbuffer +
        context->cmdbufferpos);
    *cmd = (rpc_data_cmd_use_device_t) {
        .header = {
            .command = VIDEO_CMD_USE_DEVICE,
        },
        .device_id = devicenum,
    };

    context->cmdbufferpos += sizeof(*cmd);
    return 0;
}

int libvideo_change_target(video_bitmap_t *bitmap)
{
    rpc_data_cmd_use_target_t* cmd = NULL;

    if (context->cmdbufferpos + sizeof(*cmd) >= context->cmdbufferlen) {
        libvideo_do_command_buffer();
    }

    cmd = (rpc_data_cmd_use_target_t*)(context->cmdbuffer +
        context->cmdbufferpos);
    *cmd = (rpc_data_cmd_use_target_t) {
        .header = {
            .command = VIDEO_CMD_USE_TARGET,
        },
        .target_id = bitmap->id,
    };

    context->cmdbufferpos += sizeof(*cmd);
    return 0;
}

int libvideo_change_color(char alpha, char red, char green, char blue)
{
    rpc_data_cmd_use_color_t* cmd = NULL;

    if (context->cmdbufferpos + sizeof(*cmd) >= context->cmdbufferlen) {
        libvideo_do_command_buffer();
    }

    cmd = (rpc_data_cmd_use_color_t*)(context->cmdbuffer +
        context->cmdbufferpos);
    *cmd = (rpc_data_cmd_use_color_t) {
        .header = {
            .command = VIDEO_CMD_USE_COLOR,
        },
        .alpha = alpha,
        .red = red,
        .green = green,
        .blue = blue,
    };

    context->cmdbufferpos += sizeof(*cmd);
    return 0;
}

int libvideo_change_rop(rop_t rop)
{
    rpc_data_cmd_use_rop_t* cmd = NULL;

    if (context->cmdbufferpos + sizeof(*cmd) >= context->cmdbufferlen) {
        libvideo_do_command_buffer();
    }

    cmd = (rpc_data_cmd_use_rop_t*)(context->cmdbuffer +
        context->cmdbufferpos);
    *cmd = (rpc_data_cmd_use_rop_t) {
        .header = {
            .command = VIDEO_CMD_USE_ROP,
        },
        .raster_op = rop,
    };

    context->cmdbufferpos += sizeof(*cmd);
    return 0;
}

int libvideo_get_command_buffer(size_t length)
{
    rpc_data_cmd_get_cmd_buffer_t rpc = {
        .header = {
            .command = VIDEO_CMD_GET_CMD_BUFFER,
        },
        .buffer_size = length,
    };

    uint32_t id = VIDEORPC_GET_DWORD(sizeof(rpc), (void*)&rpc);
    if (id != -1) {
        context->cmdbufferid = id;
        context->cmdbuffer = open_shared_memory(id);
        // BufferLen in Bytes! // DWords
        context->cmdbufferlen = length; // / sizeof(uint32_t);
        context->cmdbufferpos = 0;
        return 0;
    } else {
        context->cmdbufferid = 0;
        context->cmdbuffer = NULL;
        context->cmdbufferlen = 0;
        context->cmdbufferpos = 0;
        return -1;
    }
}

int libvideo_do_command_buffer()
{
    rpc_data_cmd_do_cmd_buffer_t rpc = {
        .header = {
            .command = VIDEO_CMD_DO_CMD_BUFFER,
        },
        .buffer_size = context->cmdbufferpos,
    };

    context->cmdbufferpos = 0;
    int result = VIDEORPC_GET_DWORD(sizeof(rpc), (void*)&rpc);
    return result;
}
