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

int rpc_cmd_create_context(pid_t pid, driver_context_t** context, uint32_t** response,
    void* data, uint32_t* response_size)
{
    driver_context_t* newcontext;
    newcontext = create_context(pid);
    **response = newcontext->id;
    return sizeof(rpc_data_cmd_header_t);
}


int rpc_cmd_use_context(pid_t pid, driver_context_t** context, uint32_t** response,
    void* data, uint32_t* response_size)
{
    // Kontext holen
    rpc_data_cmd_use_context_t* rpc = data;
    driver_context_t* switchcontext;
    switchcontext = get_context_by_id(rpc->context_id);
    // Berechtigung pruefen
    if (switchcontext) {
        if (switchcontext->owner == pid) {
            set_current_context_of_pid(pid, switchcontext);
            *context = switchcontext;
            **response = -EVID_NONE;
        }
        else {
            **response = -EVID_NOTALLOWED;
        }
    }
    else {
        **response = -EVID_NOCONTEXT;
    }
    return sizeof(rpc_data_cmd_use_context_t);
}

int rpc_cmd_use_device(pid_t pid, driver_context_t** context, uint32_t** response,
    void* data, uint32_t* response_size)
{
    rpc_data_cmd_use_device_t *rpc = data;
    stdout = 0;
    printf("[RPC] USE_DEVICE RUNNING\n");

    if ((*context) == NULL) {
        printf("Fehler, kein Kontext in cdi.video! UseDev\n"
            "\tVIDEO_CMD_USE_DEVICE\n");
    } else if (rpc->device_id < cdi_list_size(videodriver->drv.devices)) {
        struct cdi_video_device* dev;
        dev = cdi_list_get(videodriver->drv.devices, rpc->device_id);
        (*context)->device = dev;
        **response = -EVID_NONE;
        printf("[RPC] Device set.\n");
    } else {
        printf("[RPC] USE_DEVICE FAILED (id: %d)\n", rpc->device_id);
        **response = -EVID_NODEVICE;
    }
    return sizeof(rpc_data_cmd_use_device_t);
}

int rpc_cmd_use_target(pid_t pid, driver_context_t** context, uint32_t** response,
    void* data, uint32_t* response_size)
{
    // FIXME: Interface verwendet nur uint32_t
    rpc_data_cmd_use_target_t *rpc = data;

    // Bitmap holen
    interfacebitmap_t* bitmap;
    bitmap = bitmap_get_by_id(rpc->target_id);
    if (bitmap == NULL) {
        **response = -EVID_FAILED;
        goto fail;
    }

    // Berechtigung pruefen
    if (bitmap->owner == pid) {
        // Setzen
        (*context)->target = bitmap->bitmap;
        **response = -EVID_NONE;
    } else {
        **response = -EVID_NOTALLOWED;
    }

fail:
    return sizeof(rpc_data_cmd_use_target_t);
}

int rpc_cmd_use_color(pid_t pid, driver_context_t** context, uint32_t** response,
    void* data, uint32_t* response_size)
{
    if ((*context) == NULL) {
        fprintf(stderr, "Fehler, kein Kontext in cdi.video! UseColor\n");
        **response = -EVID_NOCONTEXT;
    } else {
        rpc_data_cmd_use_color_t *rpc = data;
        (*context)->color.alpha = rpc->alpha;
        (*context)->color.red   = rpc->red;
        (*context)->color.green = rpc->green;
        (*context)->color.blue  = rpc->blue;
        **response = -EVID_NONE;
    }
    return sizeof(rpc_data_cmd_use_color_t);
}

int rpc_cmd_use_rop(pid_t pid, driver_context_t** context, uint32_t** response,
    void* data, uint32_t* response_size)
{
    rpc_data_cmd_use_rop_t *rpc = data;
    if ((*context) == NULL) {
        fprintf(stderr, "Fehler, kein Kontext in cdi.video! UseROP\n");
        **response = -EVID_NOCONTEXT;
    } else {
        // FIXME: rop irgendwie validieren
        (*context)->rop = rpc->raster_op;
        videodriver->set_raster_op((*context)->device, (*context)->rop);
        **response = -EVID_NONE;
    }
    return sizeof(rpc_data_cmd_use_rop_t);
}

int rpc_cmd_get_cmd_buffer(pid_t pid, driver_context_t** context,
    uint32_t** response, void* data, uint32_t* response_size)
{
    if ((*context) == NULL) {
        fprintf(stderr, "Fehler, kein Kontext in cdi.video! GetCMDBuffer\n");
        **response = -EVID_NOCONTEXT;
    } else {
        rpc_data_cmd_get_cmd_buffer_t *rpc = data;
        if ((*context)->cmd_buffer_id) {
            close_shared_memory((*context)->cmd_buffer_id);
            (*context)->cmd_buffer_size = 0;
        }
        (*context)->cmd_buffer_id = create_shared_memory(rpc->buffer_size);
        if ((*context)->cmd_buffer_id) {
            (*context)->cmd_buffer = open_shared_memory((*context)->cmd_buffer_id);
            printf("RPC: cmd_buffer at 0x%x\n", (*context)->cmd_buffer);
            (*context)->cmd_buffer_size = rpc->buffer_size;
            **response = (*context)->cmd_buffer_id;
        } else {
            **response = -EVID_FAILED;
        }
    }
    return sizeof(rpc_data_cmd_get_cmd_buffer_t);
}
