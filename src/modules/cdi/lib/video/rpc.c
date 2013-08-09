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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <lostio.h>
#include <rpc.h>
#include <syscall.h>

#include <cdi.h>
#include <cdi/video.h>
#include <cdi/misc.h>
#include "video.h"
#include "context.h"
#include "bitmap.h"
#include <video/commands.h>
#include <video/error.h>

// rpc.c
int rpc_cmd_do_cmd_buffer(pid_t pid, driver_context_t** context, void** response,
    void* data, uint32_t* response_size);

// rpc_context.c
int rpc_cmd_create_context(pid_t pid, driver_context_t** context, void** response,
    void* data, uint32_t* response_size);
int rpc_cmd_use_device(pid_t pid, driver_context_t** context, void** response,
    void* data, uint32_t* response_size);
int rpc_cmd_use_context(pid_t pid, driver_context_t** context, void** response,
    void* data, uint32_t* response_size);
int rpc_cmd_use_target(pid_t pid, driver_context_t** context, void** response,
    void* data, uint32_t* response_size);
int rpc_cmd_use_color(pid_t pid, driver_context_t** context, void** response,
    void* data, uint32_t* response_size);
int rpc_cmd_use_rop(pid_t pid, driver_context_t** context, void** response,
    void* data, uint32_t* response_size);
int rpc_cmd_get_cmd_buffer(pid_t pid, driver_context_t** context, void** response,
    void* data, uint32_t* response_size);

// rpc_bitmap.c
int rpc_cmd_create_bitmap(pid_t pid, driver_context_t** context, void** response,
    void* data, uint32_t* response_size);
int rpc_cmd_destroy_bitmap(pid_t pid, driver_context_t** context, void** response,
    void* data, uint32_t* response_size);
int rpc_cmd_create_frontbuffer_bitmap(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size);
int rpc_cmd_request_backbuffers(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size);
int rpc_cmd_flip_buffers(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size);
int rpc_cmd_fetch_format(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size);
int rpc_cmd_upload_bitmap(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size);
int rpc_cmd_get_upload_buffer(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size);
int rpc_cmd_close_upload_buffer(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size);

// rpc_device.c
int rpc_cmd_get_num_devices(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size);
int rpc_cmd_get_num_displays(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size);
int rpc_cmd_set_mode(pid_t pid, driver_context_t** context, void** response,
    void* data, uint32_t* response_size);
int rpc_cmd_set_text_mode(pid_t pid, driver_context_t** context, void** response,
                          void* data, uint32_t* response_size);
int rpc_cmd_get_display_modes(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size);

// rpc_drawing.c
int rpc_cmd_draw_pixel(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size);
int rpc_cmd_draw_rectangle(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size);
int rpc_cmd_draw_line(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size);
int rpc_cmd_draw_ellipse(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size);
int rpc_cmd_draw_bitmap(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size);
int rpc_cmd_draw_bitmap_part(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size);


int (*callbacks[VIDEO_CMD_LATEST])
    (pid_t pid, driver_context_t** context, void** response, void* data,
    uint32_t* response_size) =
{
    [VIDEO_CMD_CREATE_CONTEXT]  = rpc_cmd_create_context,
    [VIDEO_CMD_USE_DEVICE]      = rpc_cmd_use_device,
    [VIDEO_CMD_USE_CONTEXT]     = rpc_cmd_use_context,
    [VIDEO_CMD_USE_TARGET]      = rpc_cmd_use_target,
    [VIDEO_CMD_USE_COLOR]       = rpc_cmd_use_color,
    [VIDEO_CMD_USE_ROP]         = rpc_cmd_use_rop,
    [VIDEO_CMD_GET_CMD_BUFFER]  = rpc_cmd_get_cmd_buffer,
    [VIDEO_CMD_DO_CMD_BUFFER]   = rpc_cmd_do_cmd_buffer,

    [VIDEO_CMD_CREATE_BITMAP]   = rpc_cmd_create_bitmap,
    [VIDEO_CMD_DESTROY_BITMAP]  = rpc_cmd_destroy_bitmap,
    [VIDEO_CMD_CREATE_FRONTBUFFER_BITMAP] = rpc_cmd_create_frontbuffer_bitmap,
    [VIDEO_CMD_REQUEST_BACKBUFFERS] = rpc_cmd_request_backbuffers,
    [VIDEO_CMD_FLIP_BUFFERS]    = rpc_cmd_flip_buffers,
    [VIDEO_CMD_FETCH_FORMAT]    = rpc_cmd_fetch_format,
    [VIDEO_CMD_GET_UPLOAD_BUFFER] = rpc_cmd_get_upload_buffer,
    [VIDEO_CMD_CLOSE_UPLOAD_BUFFER] = rpc_cmd_close_upload_buffer,
    [VIDEO_CMD_UPLOAD_BITMAP] = rpc_cmd_upload_bitmap,

    [VIDEO_CMD_GET_NUM_DEVICES] = rpc_cmd_get_num_devices,
    [VIDEO_CMD_GET_NUM_DISPLAYS]= rpc_cmd_get_num_displays,
    [VIDEO_CMD_SET_MODE]        = rpc_cmd_set_mode,
    [VIDEO_CMD_RESTORE_TEXT_MODE] = rpc_cmd_set_text_mode,
    [VIDEO_CMD_GET_DISPLAY_MODES] = rpc_cmd_get_display_modes,

    [VIDEO_CMD_DRAW_PIXEL]      = rpc_cmd_draw_pixel,
    [VIDEO_CMD_DRAW_RECTANGLE]  = rpc_cmd_draw_rectangle,
    [VIDEO_CMD_DRAW_LINE]       = rpc_cmd_draw_line,
    [VIDEO_CMD_DRAW_ELLIPSE]    = rpc_cmd_draw_ellipse,
    [VIDEO_CMD_DRAW_BITMAP]     = rpc_cmd_draw_bitmap,
    [VIDEO_CMD_DRAW_BITMAP_PART]= rpc_cmd_draw_bitmap_part,
};


int do_command_batch(pid_t pid, void* data, size_t data_size, void** result,
    uint32_t* result_size)
{
    uint32_t* cmd = (uint32_t*)data;
    int status = 0;

    driver_context_t** context = NULL;
    context = malloc(sizeof(driver_context_t*));
    *context = get_current_context_of_pid(pid);

    while (data_size > 0) {
        if (cmd[0] >= VIDEO_CMD_LATEST) {
            printf("Process %d issued an invalid cmd! Aborting.\n", pid);
            break;
        }
        if (callbacks[cmd[0]]) {
            int used_data = 0;
            used_data = (*callbacks[cmd[0]])(pid, context, (void**)result,
&(cmd[0]), result_size);
            //printf("CMD: %d\n", cmd[0]);
            //used_data += sizeof(uint32_t);
            cmd = (uint32_t*)((char*)cmd + used_data);
            data_size -= used_data;
        } else {
            printf("Fehler: Unbekanntes Kommando: %d\n", cmd[0]);
            return -EVID_INVLDCMD; // FIXME Korrekte Fehlerkonstante
        }
    }


    return status;
}

void rpc_video_interface(pid_t pid,
                          uint32_t correlation_id,
                          size_t data_size,
                          void* data)
{
    uint32_t default_response_holder;
    uint32_t* response = &default_response_holder;
    uint32_t response_size = sizeof(uint32_t);

    stdout = 0;
    do_command_batch(pid, data, data_size, (void**)&response, &response_size);
    rpc_send_response(pid, correlation_id, response_size, (char*)response);

    if (response != &default_response_holder) {
        free(response);
    }
    return;
}

int rpc_cmd_do_cmd_buffer(pid_t pid, driver_context_t** context, void** response,
    void* data, uint32_t* response_size)
{
    rpc_data_cmd_do_cmd_buffer_t* rpc = (rpc_data_cmd_do_cmd_buffer_t*)data;

    size_t data_size = rpc->buffer_size;
    void* cmd_start = (*context)->cmd_buffer;
    do_command_batch(pid, cmd_start, data_size, response, response_size);
    // FIXME: Sinnvolle Fehlerkonstanten ueberlegen.
    **(uint32_t**)response = -EVID_NONE;
    return sizeof(*rpc);
}
