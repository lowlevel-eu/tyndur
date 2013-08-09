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
#include <string.h>
#include <video/commands.h>
#include <video/error.h>
#include <cdi/mem.h>

interfacebitmap_t* frontbuffer_bitmap;

int rpc_cmd_create_bitmap(pid_t pid, driver_context_t** context, uint32_t** response,
    void* data, uint32_t* response_size)
{
    rpc_data_cmd_create_bitmap_t *rpc = data;

    if ((*context) == NULL) {
        fprintf(stderr, "Fehler, kein Kontext in cdi.video!\n");
    } else {
        struct cdi_video_bitmap* cdibitmap;
        if (rpc->data_length > 0) {
            cdibitmap = videodriver->bitmap_create((*context)->device,
                rpc->width, rpc->height,
                (void*)data + sizeof(rpc_data_cmd_create_bitmap_t));
            /// FIXME: Datenuebergabe aus Shared Memory, RPC zu klein!
            /// Ggf. eigene Funktion UPLOAD_BITMAP_DATA?
        } else {
            cdibitmap = videodriver->bitmap_create((*context)->device,
                rpc->width, rpc->height, NULL);
        }
        if (cdibitmap != NULL) {
            interfacebitmap_t* bitmap;
            bitmap = bitmap_create(cdibitmap, pid);
            if (bitmap) {
                // FIXME: id ist uint64_t
                **response = bitmap->id;
            } else {
                **response = -EVID_FAILED;
            }
        } else {
            **response = -EVID_FAILED;
        }
    }
    return sizeof(rpc_data_cmd_create_bitmap_t) + rpc->data_length;
}

int rpc_cmd_destroy_bitmap(pid_t pid, driver_context_t** context, uint32_t** response,
    void* data, uint32_t* response_size)
{
    rpc_data_cmd_destroy_bitmap_t *rpc = data;

    interfacebitmap_t* bitmap;
    bitmap = bitmap_get_by_id(rpc->bitmap_id);
    if (bitmap->owner == pid) {
        if (bitmap == frontbuffer_bitmap) {
            frontbuffer_bitmap = NULL;
        }
        stdout = 0;
        printf("RPC-Interface: Free Bitmap 0x%08X\n", bitmap->bitmap);
        videodriver->bitmap_destroy(bitmap->bitmap);
        bitmap_destroy(bitmap);
        **response = -EVID_NONE;
    } else {
        **response = -EVID_NOTALLOWED;
    }
    return sizeof(rpc_data_cmd_destroy_bitmap_t);
}

int rpc_cmd_create_frontbuffer_bitmap(pid_t pid, driver_context_t** context,
    uint32_t** response, void* data, uint32_t* response_size)
{
    rpc_data_cmd_create_frontbuffer_bitmap_t *rpc = data;

    if ((*context) == NULL) {
        fprintf(stderr, "Fehler, kein Kontext in cdi.video!\n");
        **response = -EVID_NOCONTEXT;
    } else if ((*context)->device == NULL) {
        fprintf(stderr, "Fehler, kein Gerät im Kontext in cdi.video!\n");
        **response = -EVID_NODEVICE;
/*    } else if (frontbuffer_bitmap) {
        **response = -1;*/
    } else if (rpc->display_id < cdi_list_size((*context)->device->displays)) {
        struct cdi_video_display* display;
        display = cdi_list_get((*context)->device->displays, rpc->display_id);
        if (display->frontbuffer == NULL) {
            // Ohne vorhandene FB-Bitmap gehts nicht weiter,
            // also Abbrechen
            **response = -EVID_FAILED;
        } else {
            interfacebitmap_t* frontbitmap;
            frontbitmap = bitmap_create(display->frontbuffer, pid);
            frontbuffer_bitmap = frontbitmap;
            **response = frontbitmap->id;
        }
    } else {
        //printf("display nicht gefunden.\n");
        **response = -EVID_NODISPLAY;
    }

    return sizeof(rpc_data_cmd_create_frontbuffer_bitmap_t);
}

int rpc_cmd_request_backbuffers(pid_t pid, driver_context_t** context,
    uint32_t** response, void* data, uint32_t* response_size)
{
    rpc_data_cmd_request_backbuffers_t *rpc = data;

    interfacebitmap_t* bitmap;
    bitmap = bitmap_get_by_id(rpc->bitmap_id);
    if (bitmap->owner == pid) {
        int result = videodriver->bitmap_request_backbuffers(bitmap->bitmap,
            rpc->backbuffer_count);
        **response = result;
    } else {
        **response = -EVID_NOTALLOWED;
    }

    return sizeof(rpc_data_cmd_request_backbuffers_t);
}

int rpc_cmd_flip_buffers(pid_t pid, driver_context_t** context,
    uint32_t** response, void* data, uint32_t* response_size)
{
    rpc_data_cmd_flip_buffers_t *rpc = data;

    interfacebitmap_t* bitmap;
    bitmap = bitmap_get_by_id(rpc->bitmap_id);
    if (bitmap->owner == pid) {
        videodriver->bitmap_flip_buffers(bitmap->bitmap);
        **response = -EVID_NONE;
    } else {
        **response = -EVID_NOTALLOWED;
    }
    return sizeof(rpc_data_cmd_flip_buffers_t);
}

int rpc_cmd_fetch_format(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size)
{
    rpc_data_cmd_fetch_format_t *rpc = data;

    interfacebitmap_t* bitmap;
    bitmap = bitmap_get_by_id(rpc->bitmap_id);
    *response_size = sizeof(struct cdi_video_pixel_format);
    void* format_buffer = calloc(1, *response_size);
    if (format_buffer != NULL) {
        memcpy(format_buffer, &(bitmap->bitmap->format), *response_size);
        *response = format_buffer;
    } else {
        **(uint32_t**)response = -EVID_FAILED;
        *response_size = sizeof(uint32_t);
    }
    return sizeof(rpc_data_cmd_fetch_format_t);
}

int rpc_cmd_upload_bitmap(pid_t pid, driver_context_t** context,
    uint32_t** response, void* data, uint32_t* response_size)
{
    rpc_data_cmd_upload_bitmap_t *rpc = data;

    interfacebitmap_t* bitmap;
    bitmap = bitmap_get_by_id(rpc->bitmap_id);
    if (bitmap->owner == pid) {
        if (bitmap->upload_buffer_id != 0) {
            size_t bitmap_size = bitmap->bitmap->pitch * bitmap->bitmap->height;
            if (bitmap_size != bitmap->upload_buffer_size) {
                // Format zwischenzeitlich geaendert? Ablehnen!
                **response = -EVID_INVLDBITMAP;
            } else {
                int x;
                char* addr = NULL;
                for (x=0; x < bitmap->bitmap->buffercount; x++) {
                    if (bitmap->bitmap->buffers[x].type & CDI_VIDEO_BUFFER_WRITE) {
                        if (bitmap->bitmap->buffers[x].in_vram) {
                            addr = bitmap->bitmap->device->vram->vaddr;
                        }
                        addr += (size_t)bitmap->bitmap->buffers[x].pixeldata;
                        break;
                    }
                }
                if (addr) {
                    memcpy(addr, bitmap->upload_buffer_ptr, bitmap_size);
                    **response = EVID_NONE;
                } else {
                    **response = -EVID_FAILED;
                }
            }
        } else {
            **response = -EVID_FAILED;
        }
    } else {
        **response = -EVID_NOTALLOWED;
    }
    return sizeof(rpc_data_cmd_upload_bitmap_t);
}

int rpc_cmd_get_upload_buffer(pid_t pid, driver_context_t** context,
    void** response, void* data, uint32_t* response_size)
{
    rpc_data_cmd_get_upload_buffer_t *rpc = data;
    interfacebitmap_t* bitmap;
    bitmap = bitmap_get_by_id(rpc->bitmap_id);
    if (bitmap->owner == pid) {
        if (bitmap->upload_buffer_id == 0) {
            size_t bitmap_size = bitmap->bitmap->pitch * bitmap->bitmap->height;
            bitmap->upload_buffer_id = create_shared_memory(bitmap_size);
            bitmap->upload_buffer_ptr = open_shared_memory(bitmap->upload_buffer_id);
            bitmap->upload_buffer_size = bitmap_size;
        }
        uint32_t* buffer_info = calloc(2, sizeof(uint32_t));
        buffer_info[0] = bitmap->upload_buffer_id;
        buffer_info[1] = bitmap->upload_buffer_size;
        printf("Send Upload Buffer, %d, size %p\n", buffer_info[0], buffer_info[1]);
        *response = buffer_info;
        *response_size = 2 * sizeof(uint32_t);
    } else {
        **(uint32_t**)response = -EVID_NOTALLOWED;
    }
    return sizeof(rpc_data_cmd_get_upload_buffer_t);
}

int rpc_cmd_close_upload_buffer(pid_t pid, driver_context_t** context,
    uint32_t** response, void* data, uint32_t* response_size)
{
    rpc_data_cmd_close_upload_buffer_t *rpc = data;

    interfacebitmap_t* bitmap;
    bitmap = bitmap_get_by_id(rpc->bitmap_id);
    if (bitmap->owner == pid) {
        if (bitmap->upload_buffer_id == 0) {
            close_shared_memory(bitmap->upload_buffer_id);
            bitmap->upload_buffer_id = 0;
            **response = 0;
        } else {
            **response = -EVID_FAILED;
        }
    } else {
        **response = -EVID_NOTALLOWED;
    }
    return sizeof(rpc_data_cmd_close_upload_buffer_t);
}
