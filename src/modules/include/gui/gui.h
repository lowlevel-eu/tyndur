/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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

#ifndef GUI_GUI_H
#define GUI_GUI_H

#include "types.h"
#include "video/bitmap.h"
#include "gui/events.h"
#include "collections.h"

typedef struct widget_t
{
    int x;
    int y;
    dword width;
    dword height;
    dword type;
    
    dword id;
    
    dword *data;
} widget_t;

typedef struct window_t
{
    int x;
    int y;
    dword width;
    dword height;
    dword flags;
    
    dword id;
    dword shmid;
    bitmap_t bitmap;
    
    list_t *widgets;
    
    char *title;
} window_t;

typedef struct font_t
{
    char *name;
    int size;
    dword id;
    dword shmid;
    byte *data;
} font_t;

#define GUI_WINDOW_FRAME 1
#define GUI_WINDOW_RESIZEABLE (1<<1)
#define GUI_WINDOW_MINIMIZE (1<<2)
#define GUI_WINDOW_MAXIMIZE (1<<3)
#define GUI_WINDOW_CLOSE (1<<4)
#define GUI_WINDOW_NO_FOCUS (1<<5)
#define GUI_WINDOW_ALWAYS_ON_BOTTOM (1<<6)
#define GUI_WINDOW_ALWAYS_ON_TOP (1<<7)
#define GUI_WINDOW_NO_BACKGROUND (1<<8)

extern font_t *titlefont;

int init_gui(void);
int close_gui(void);

window_t *gui_create_window(int x, int y, int width, int height, dword flags);
void gui_delete_window(window_t *window);

void gui_repaint_window(window_t *window);

void gui_start_moving_window(window_t *window);
void gui_start_resizing_window(window_t *window, dword position);

void gui_set_window_title(window_t *window, const char *title);
const char *gui_get_window_title(window_t *window);

void gui_send_event(event_t event);

void gui_set_callback(void (*cb)(event_t));

font_t *gui_get_font(const char *name, int size);
void gui_delete_font(font_t *font);

int render_char(bitmap_t bitmap, font_t *font, unsigned char c, int x, int y);
void render_text(bitmap_t bitmap, font_t *font, const char *string, int x, int y);
dword get_text_width(font_t *font, const char *string);
dword get_string_position(font_t *font, const char *string, int x);

#endif
