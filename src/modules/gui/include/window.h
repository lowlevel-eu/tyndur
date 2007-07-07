/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Mathias Gottschlag.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WINDOW_H
#define WINDOW_H

#include "types.h"
#include "video/bitmap.h"
#include "gui/events.h"
#include "gui/gui.h"

typedef struct event_receiver_t
{
    pid_t owner;
    char name[10];
} event_receiver_t;

typedef struct server_window_t
{
    int x;
    int y;
    dword width;
    dword height;
    dword flags;
    
    dword needs_repainting;
    
    pid_t owner;
    dword id;
    dword shmid;
    bitmap_t bitmap;
} server_window_t;

extern dword moving_window;
extern dword resizing_window;

void init_windows(void);

server_window_t *create_window(pid_t pid, int x, int y, dword width, dword height, dword flags);
dword delete_window(pid_t pid, dword windowid);
dword redraw_window(dword windowid);
void draw_all_windows(void);
void set_event_receiver(pid_t pid, char *name);

int start_moving_window(dword windowid);
int start_resizing_window(dword windowid, dword position);
void stop_resizing_window(void);

server_window_t *get_window_at_position(int x, int y);

void send_event(pid_t pid, event_t event);

void inject_mouse_move(int x, int y);
void inject_mouse_button_pressed(int button);
void inject_mouse_button_released(int button);

#endif
