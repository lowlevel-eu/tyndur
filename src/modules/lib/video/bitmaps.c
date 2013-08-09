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
#include <string.h>

extern driver_context_t* context;

video_bitmap_t* libvideo_create_bitmap(int width, int height,
    size_t data_length, void* data)
{
    video_bitmap_t *bitmap = calloc(1, sizeof(video_bitmap_t));
    if (bitmap == NULL) {
        return NULL;
    }

    rpc_data_cmd_create_bitmap_t rpc = {
        .header = {
            .command = VIDEO_CMD_CREATE_BITMAP,
        },
        .width = width,
        .height = height,
        .data_length = 0,
    };

    bitmap->id = VIDEORPC_GET_DWORD(sizeof(rpc), (void*)&rpc);

    if (bitmap->id) {
        bitmap->width = width;
        bitmap->height = height;
        bitmap->upload_buffer_id = 0;
        bitmap->upload_buffer_size = 0;
        bitmap->upload_buffer_ptr = NULL;
        return bitmap;
    } else {
        free(bitmap);
    }
    return NULL;
}

void libvideo_destroy_bitmap(video_bitmap_t *bitmap)
{
    rpc_data_cmd_destroy_bitmap_t rpc = {
        .header = {
            .command = VIDEO_CMD_DESTROY_BITMAP,
        },
        .bitmap_id = bitmap->id,
    };

    free(bitmap);
    VIDEORPC_GET_DWORD(sizeof(rpc), (void*)&rpc);
}

video_bitmap_t* libvideo_get_frontbuffer_bitmap(int display)
{
    rpc_data_cmd_create_frontbuffer_bitmap_t rpc = {
        .header = {
            .command = VIDEO_CMD_CREATE_FRONTBUFFER_BITMAP,
        },
        .display_id = display,
    };

    uint32_t bitmapid = VIDEORPC_GET_DWORD(sizeof(rpc), (void*)&rpc);
    if (bitmapid) {
        video_bitmap_t *bitmap = calloc(1, sizeof(video_bitmap_t));
        if (bitmap) {
            bitmap->id = bitmapid;
            // FIXME: Breite und Höhe müssten irgendwie gesetzt werden.
            return bitmap;
        }
    }
    return NULL;
}

void libvideo_flip_buffers(video_bitmap_t* bitmap)
{
    rpc_data_cmd_flip_buffers_t rpc = {
        .header = {
            .command = VIDEO_CMD_FLIP_BUFFERS,
        },
        .bitmap_id = bitmap->id,
    };

    VIDEORPC_GET_DWORD(sizeof(rpc), (void*)&rpc);
}

uint32_t libvideo_request_backbuffers(video_bitmap_t* bitmap,
    uint32_t backbuffer_count)
{
    rpc_data_cmd_request_backbuffers_t rpc = {
        .header = {
            .command = VIDEO_CMD_REQUEST_BACKBUFFERS,
        },
        .bitmap_id = bitmap->id,
        .backbuffer_count = backbuffer_count,
    };

    return VIDEORPC_GET_DWORD(sizeof(rpc), (void*)&rpc);
}

uint32_t libvideo_upload_bitmap(video_bitmap_t* bitmap)
{
    if (!bitmap) {
        return 0;
    }

    rpc_data_cmd_upload_bitmap_t rpc = {
        .header = {
            .command = VIDEO_CMD_UPLOAD_BITMAP,
        },
        .bitmap_id = bitmap->id,
    };

    return VIDEORPC_GET_DWORD(sizeof(rpc), (void*)&rpc);
}

uint32_t libvideo_close_upload_buffer(video_bitmap_t* bitmap)
{
    if (!bitmap) {
        return 0;
    }
    if (bitmap->upload_buffer_id) {
        close_shared_memory(bitmap->upload_buffer_id);
    }

    rpc_data_cmd_close_upload_buffer_t rpc = {
        .header = {
            .command = VIDEO_CMD_CLOSE_UPLOAD_BUFFER,
        },
        .bitmap_id = bitmap->id,
    };

    uint32_t result = VIDEORPC_GET_DWORD(sizeof(rpc), (void*)&rpc);
    return result;
}

void* libvideo_open_upload_buffer(video_bitmap_t* bitmap, size_t* buffer_size)
{
    if (!bitmap) {
        return NULL;
    }

    rpc_data_cmd_get_upload_buffer_t rpc = {
        .header = {
            .command = VIDEO_CMD_GET_UPLOAD_BUFFER,
        },
        .bitmap_id = bitmap->id,
    };

    response_t *resp;
    resp = rpc_get_response(context->driverpid, "VIDEODRV", sizeof(rpc),
        (void*)&rpc);

    uint32_t* data = resp->data;
    bitmap->upload_buffer_id = data[0];
    if (bitmap->upload_buffer_id) {
        bitmap->upload_buffer_size = data[1];
        bitmap->upload_buffer_ptr = open_shared_memory(bitmap->upload_buffer_id);
        if (buffer_size) {
            *buffer_size = bitmap->upload_buffer_size;
        }
        printf("Got Upload Buffer %d at: %p | size: %p\n", data[0], bitmap->upload_buffer_ptr ,data[1]);
        return bitmap->upload_buffer_ptr;
    }
    return NULL;
}

video_pixel_format_t* libvideo_fetch_bitmap_format(video_bitmap_t* bitmap)
{
    if (!bitmap) {
        return NULL;
    }

    rpc_data_cmd_fetch_format_t rpc = {
        .header = {
            .command = VIDEO_CMD_FETCH_FORMAT,
        },
        .bitmap_id = bitmap->id,
    };

    response_t *resp;
    resp = rpc_get_response(context->driverpid, "VIDEODRV", sizeof(rpc),
        (void*)&rpc);
    uint8_t* data = resp->data;
    // Ersten 8 bit sind BPP
    if (*data) {
        return resp->data;
    }
    return NULL;
}
