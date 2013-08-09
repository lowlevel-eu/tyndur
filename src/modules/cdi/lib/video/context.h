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

#ifndef __CDI_VIDEO_LIB_CONTEXT_H__
#define __CDI_VIDEO_LIB_CONTEXT_H__

#include <cdi/lists.h>
#include <cdi/video.h>
#include <types.h>

extern int context_ctr;
extern cdi_list_t context_list;
extern cdi_list_t user_list;

typedef struct {
    pid_t owner;
    int id;
    struct cdi_video_device* device;
    struct cdi_video_bitmap* target;
    struct cdi_video_color color;
    enum cdi_video_raster_op rop;
    uint32_t* cmd_buffer;
    uint32_t cmd_buffer_id;
    size_t cmd_buffer_size;
} driver_context_t;

typedef struct {
    pid_t owner;
    driver_context_t* context;
} pid_context_t;



driver_context_t* get_current_context_of_pid(pid_t pid);
int set_current_context_of_pid(pid_t pid, driver_context_t* context);
driver_context_t* get_context_by_id(int id);
driver_context_t* create_context(pid_t pid);
void context_initialize(void);

#endif /* __CDI_VIDEO_LIB_CONTEXT_H__ */
