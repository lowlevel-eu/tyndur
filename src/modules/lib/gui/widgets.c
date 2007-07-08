/*  
 * Copyright (c) 2007 The LOST Project. All rights reserved.
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

#include "gui/widgets.h"

#include "string.h"
#include "stdlib.h"
#include "stdio.h"

int lastid = 0;

widget_t *create_button(window_t *window, int x, int y, int width, int height, char *label)
{
    widget_t *widget = malloc(sizeof(widget_t));
    widget->x = x;
    widget->y = y;
    widget->width = width;
    widget->height = height;
    widget->type = GUI_WIDGET_BUTTON;
    widget->id = ++lastid;
    widget->data = malloc(4 + strlen(label) + 1);
    widget->data[0] = 0;
    strcpy(((char*)widget->data) + 4, label);
    list_push(window->widgets, widget);
    return widget;
}

int widget_is_at_position(widget_t *widget, int x, int y)
{
    if ((x > widget->x) && (x < widget->x + widget->width)
     && (y > widget->y) && (y < widget->y + widget->height)) {
        return 1;
    }
    return 0;
}

void widget_process_event(window_t *window, widget_t *widget, event_t event)
{
    switch (widget->type)
    {
        case GUI_WIDGET_BUTTON:
            if (event.type == EVENT_MOUSE_BUTTON_PRESSED) {
                widget->data[0] = 1;
                gui_repaint_window(window);
            } else if (event.type == EVENT_MOUSE_BUTTON_RELEASED) {
                if (widget->data[0] == 1) {
                    widget->data[0] = 0;
                    event_t newevent;
                    newevent.type = EVENT_BUTTON_PRESSED;
                    newevent.data[0] = widget->id;
                    newevent.data[1] = (dword)widget;
                    gui_send_event(newevent);
                    gui_repaint_window(window);
                }
            }
            //TODO: Button mit der Maus verlassen deaktiviert ihn
            break;
        case GUI_WIDGET_EDITBOX:
            break;
        case GUI_WIDGET_LABEL:
            break;
    }
}

int process_event(window_t *window, event_t event)
{
    int i;
    switch (event.type)
    {
        case EVENT_MOUSE_BUTTON_PRESSED:
            //Event an richtiges Widget geben
            for (i = 0; i < list_size(window->widgets); i++) {
                widget_t *widget = (widget_t*)list_get_element_at(window->widgets, i);
                if (widget_is_at_position(widget, event.data[1], event.data[2])) {
                    widget_process_event(window, widget, event);
                    return 1;
                }
            }
            //TODO: Klick ins Nichts = Kein Fokus mehr
            break;
        case EVENT_MOUSE_BUTTON_RELEASED:
            for (i = 0; i < list_size(window->widgets); i++) {
                widget_t *widget = (widget_t*)list_get_element_at(window->widgets, i);
                if (widget_is_at_position(widget, event.data[1], event.data[2])) {
                    widget_process_event(window, widget, event);
                    return 1;
                }
            }
            break;
    }
    return 0;
}

void draw_widgets(window_t *window)
{
    int i;
    for (i = 0; i < list_size(window->widgets); i++) {
        widget_t *widget = (widget_t*)list_get_element_at(window->widgets, i);
        switch (widget->type)
        {
            case GUI_WIDGET_BUTTON:
                bitmap_draw_rect(window->bitmap, 0xCCCCCC, widget->x, widget->y,
                    widget->width, widget->height);
                if (widget->data[0]) {
                    bitmap_draw_rect(window->bitmap, 0x555555, widget->x, widget->y,
                        widget->width, 1);
                    bitmap_draw_rect(window->bitmap, 0x555555, widget->x, widget->y,
                        1, widget->height);
                    bitmap_draw_rect(window->bitmap, 0xEEEEEE, widget->x, widget->y + widget->height - 1,
                        widget->width, 1);
                    bitmap_draw_rect(window->bitmap, 0xEEEEEE, widget->x + widget->width - 1, widget->y,
                        1, widget->height);
                } else {
                    bitmap_draw_rect(window->bitmap, 0xEEEEEE, widget->x, widget->y,
                        widget->width, 1);
                    bitmap_draw_rect(window->bitmap, 0xEEEEEE, widget->x, widget->y,
                        1, widget->height);
                    bitmap_draw_rect(window->bitmap, 0x555555, widget->x, widget->y + widget->height - 1,
                        widget->width, 1);
                    bitmap_draw_rect(window->bitmap, 0x555555, widget->x + widget->width - 1, widget->y,
                        1, widget->height);
                }
                break;
            case GUI_WIDGET_EDITBOX:
                break;
            case GUI_WIDGET_LABEL:
                break;
        }
    }
}
