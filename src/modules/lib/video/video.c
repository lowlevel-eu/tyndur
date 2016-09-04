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
#include <services.h>
#include <rpc.h>
#include <stdlib.h>
#include <video/commands.h>

driver_context_t* context = NULL;

driver_context_t* libvideo_create_driver_context(char* driver)
{
    driver_context_t* newcontext;

    if (!servmgr_need(driver)) {
        return NULL;
    }

    newcontext = calloc(1, sizeof(driver_context_t));
    newcontext->driverpid = init_service_get(driver);

    rpc_data_cmd_header_t rpc = {
        .command = VIDEO_CMD_CREATE_CONTEXT,
    };

    newcontext->drivercontext = rpc_get_dword(newcontext->driverpid, "VIDEODRV",
        sizeof(rpc), (void*)&rpc);

    printf("Context ID: %d\n",newcontext->drivercontext);
    libvideo_use_driver_context(newcontext);

    return newcontext;
}

inline void libvideo_use_driver_context(driver_context_t* newcontext)
{
    context = newcontext;

    rpc_data_cmd_use_context_t rpc_use_context = {
        .header = {
            .command = VIDEO_CMD_USE_CONTEXT,
        },
        .context_id = context->drivercontext,
    };

    rpc_get_dword(newcontext->driverpid, "VIDEODRV", sizeof(rpc_use_context),
        (void*)&rpc_use_context);
}

int libvideo_change_display_resolution(int display, int width, int height,
    int depth)
{
    rpc_data_cmd_set_mode_t rpc = {
        .header = {
            .command = VIDEO_CMD_SET_MODE,
        },
        .display = display,
        .width = width,
        .height = height,
        .depth = depth,
        .refreshrate = 0, /* AUTO */
    };

    return rpc_get_dword(context->driverpid, "VIDEODRV", sizeof(rpc),
        (void*)&rpc);
}

int libvideo_restore_text_mode(void)
{
    rpc_data_cmd_header_t rpc = {
        .command = VIDEO_CMD_RESTORE_TEXT_MODE,
    };
    return rpc_get_dword(context->driverpid, "VIDEODRV", sizeof(rpc),
        (void*)&rpc);
}
