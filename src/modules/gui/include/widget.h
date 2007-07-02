/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Antoine Kaufmann.
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
 
#ifndef WIDGET_H
#define WIDGET_H

#include "widgetclass.h"
#include "font.h"

extern font_t default_font;

#define EVENT_MOUSE_MOVED 0
#define EVENT_MOUSE_ENTERED_AREA 1
#define EVENT_MOUSE_LEFT_AREA 2
#define EVENT_MOUSE_BUTTON_PRESSED 3
#define EVENT_MOUSE_BUTTON_RELEASED 4

#define EVENT_BUTTON_CLICKED 5

int init_root_widget(void);

void set_default_font(font_t font);

//Widgetklassen
int register_widget_class(int owner, widget_class_t class);
int delete_widget_class(char *name);
widget_class_t *get_widget_class(char *name);

//Management
int create_widget_id(void);
void delete_widget_id(int id);
widget_t get_widget(int id);
int get_widget_index(int id);
widget_t create_widget(int id, char *widgetclass, int parentid, int process);
widget_t create_widget_ex(int id, char *widgetclass, int  parentid, position_t position, dimension_t size, int process);
void delete_widget(int id);

void add_child(widget_t parent, widget_t child);
void delete_child(widget_t parent, widget_t child);
void move_to_front(widget_t widget);

void start_dragging(widget_t widget);

//Eigenschaften
void set_position(int id, position_t position);
void set_size(int id, dimension_t size);
void set_label(int id, char *label);

//Rendern
int draw_widget(widget_t widget);
int draw_widget_recursive(widget_t widget);
int draw_all(void);

widget_t get_widget_at_position(widget_t parent, int x, int y);

//Input
void inject_mouse_move(int x, int y);
void inject_mouse_button_pressed(int button);
void inject_mouse_button_released(int button);

void inject_mouse_event(widget_t widget, event_t event);

//Callback
int register_callback_function(int process, char *name);
int delete_callback_function(int process);
void send_gui_event(int process, event_t event);

#endif
