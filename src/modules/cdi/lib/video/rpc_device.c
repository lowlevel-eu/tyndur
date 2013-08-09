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
#include <stdlib.h>
#include <video/commands.h>
#include <video/error.h>

extern interfacebitmap_t* frontbuffer_bitmap;

int rpc_cmd_get_num_devices(pid_t pid, driver_context_t** context, uint32_t** response,
    void* data, uint32_t* response_size)
{
    **response = cdi_list_size(videodriver->drv.devices);
    return sizeof(uint32_t);
}

int rpc_cmd_get_num_displays(pid_t pid, driver_context_t** context, uint32_t** response,
    void* data, uint32_t* response_size)
{
    rpc_data_cmd_get_num_displays_t *rpc = data;
    if (rpc->device_id < cdi_list_size(videodriver->drv.devices)) {
        struct cdi_video_device* dev;
        dev = cdi_list_get(videodriver->drv.devices, rpc->device_id);
        **response = cdi_list_size(dev->displays);
    } else {
        **response = -EVID_NODEVICE;
    }
    return sizeof(rpc_data_cmd_get_num_displays_t);
}


int rpc_cmd_set_mode(pid_t pid, driver_context_t** context, uint32_t** response,
    void* data, uint32_t* response_size)
{
    rpc_data_cmd_set_mode_t *rpc = data;

    if ((*context) == NULL) {
        fprintf(stderr, "Fehler, kein Kontext in cdi.video! SM, Context\n");
        **response = -EVID_NOCONTEXT;
    } else if ((*context)->device == NULL) {
        fprintf(stderr, "Fehler, kein Kontext in cdi.video! SM, Device\n");
        **response = -EVID_NODEVICE;
    } else if (frontbuffer_bitmap) {
        **response = -EVID_FAILED;
    } else {
        struct cdi_video_displaymode* mode = NULL;
        struct cdi_video_displaymode* best_match = NULL;
        if (rpc->display < cdi_list_size((*context)->device->displays)) {
            struct cdi_video_display* display;
            display = cdi_list_get((*context)->device->displays, rpc->display);
            int i;
            for (i = 0; i < cdi_list_size(display->modes); i++) {
                mode = cdi_list_get(display->modes, i);
                if (mode->width == rpc->width &&
                    mode->height == rpc->height &&
                    mode->depth == rpc->depth &&
                    mode->refreshrate == rpc->refreshrate)
                {
                    // Wir haben unseren Modus
                    best_match = mode;
                    break;
                } else if (mode->width == rpc->width &&
                    mode->height == rpc->height &&
                    (!best_match  || mode->depth >= best_match->depth) && mode->depth >= rpc->depth)
                {
                    best_match = mode;
                }
            }
            if (best_match) {
                videodriver->display_set_mode((*context)->device, display, mode);
                **response = -EVID_NONE;
            } else {
                **response = -EVID_FAILED;
            }
        }
    }
    return sizeof(rpc_data_cmd_set_mode_t);
}

int rpc_cmd_set_text_mode(pid_t pid, driver_context_t** context, uint32_t** response,
                     void* data, uint32_t* response_size)
{
    if ((*context) == NULL) {
        fprintf(stderr, "Fehler, kein Kontext in cdi.video! STM, Context\n");
        **response = -EVID_NOCONTEXT;
    } else if ((*context)->device == NULL) {
        fprintf(stderr, "Fehler, kein Kontext in cdi.video! STM, Device\n");
        **response = -EVID_NODEVICE;
    } else if (frontbuffer_bitmap) {
        stdout = 0;
        printf("Frontbuffer exists!\n");
        **response = -EVID_FAILED;
    } else {
            stdout = 0;
            printf("(RPC) try setting text mode\n");
            videodriver->restore_text_mode((*context)->device);
            **response = -EVID_NONE;
    }
    return sizeof(rpc_data_cmd_header_t);
}


int rpc_cmd_get_display_modes(pid_t pid, driver_context_t** context,
    uint32_t** response, void* data, uint32_t* response_size)
{
    rpc_data_cmd_get_display_modes_t *rpc = data;

    struct cdi_video_device* dev;
    if (rpc->device_id < cdi_list_size(videodriver->drv.devices)) {
        dev = cdi_list_get(videodriver->drv.devices, rpc->device_id);
    } else {
        **response = -EVID_NODEVICE;
        return sizeof(rpc_data_cmd_get_display_modes_t);
    }

    struct cdi_video_display* display;
    if (rpc->display_id < cdi_list_size(dev->displays)) {
        display = cdi_list_get(dev->displays, rpc->display_id);
    }
    else {
        **response = -EVID_NODISPLAY;
        return sizeof(rpc_data_cmd_get_display_modes_t);
    }

    int num_modes = cdi_list_size(display->modes);
    if (num_modes >= 0) {
        size_t modelist_size = sizeof(uint32_t) +
            sizeof(uint32_t) * num_modes * 4;
        uint32_t *modelist = calloc(1, modelist_size);
        modelist[0] = num_modes;
        int i;
        for (i = 0; i < cdi_list_size(display->modes); i++) {
            struct cdi_video_displaymode* mode;
            mode = cdi_list_get(display->modes, i);
            modelist[1 + i*4] = mode->width;
            modelist[1 + i*4 + 1] = mode->height;
            modelist[1 + i*4 + 2] = mode->depth;
            modelist[1 + i*4 + 3] = mode->refreshrate;
        }
        *response = modelist;
        *response_size = modelist_size;
    }
    return sizeof(rpc_data_cmd_get_display_modes_t);

}
