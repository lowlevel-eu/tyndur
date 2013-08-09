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
#include <string.h>
#include <init.h>
#include <rpc.h>
#include <stdlib.h>
#include <video/commands.h>
#include <syscall.h>

extern driver_context_t* context;

int libvideo_get_num_devices(void)
{
    rpc_data_cmd_header_t rpc = {
        .command = VIDEO_CMD_GET_NUM_DEVICES,
    };
    return VIDEORPC_GET_DWORD(sizeof(rpc), (void*)&rpc);
}

int libvideo_get_num_displays(int device)
{
    rpc_data_cmd_get_num_displays_t rpc = {
        .header = {
            .command = VIDEO_CMD_GET_NUM_DISPLAYS,
        },
        .device_id = device,
    };
    return VIDEORPC_GET_DWORD(sizeof(rpc), (void*)&rpc);
}

uint32_t libvideo_get_modes(int device, int display, video_mode_t** modeptr)
{
    rpc_data_cmd_get_display_modes_t rpc = {
        .header = {
            .command = VIDEO_CMD_GET_DISPLAY_MODES,
        },
        .device_id = device,
        .display_id = display,
    };
    response_t *resp;
    resp = rpc_get_response(context->driverpid, "VIDEODRV", sizeof(rpc),
        (void*)&rpc);
    uint32_t modecnt = *(uint32_t*)resp->data;
    if (modeptr) {
        *modeptr = calloc(*(uint32_t*)resp->data, sizeof(video_mode_t));
        memcpy(*modeptr, (char*)resp->data + sizeof(uint32_t),
            modecnt * sizeof(video_mode_t));
    }
    free(resp->data);
    free(resp);
    return modecnt;
}
