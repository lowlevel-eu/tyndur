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

widget_t *focus = 0;

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

widget_t *create_edit_box(window_t *window, int x, int y, int width, int height, char *value)
{
    widget_t *widget = malloc(sizeof(widget_t));
    widget->x = x;
    widget->y = y;
    widget->width = width;
    widget->height = height;
    widget->type = GUI_WIDGET_EDITBOX;
    widget->id = ++lastid;
    widget->data = malloc(4 + strlen(value) + 1);
    widget->data[0] = 0;
    strcpy(((char*)widget->data) + 4, value);
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
            if (event.type == EVENT_MOUSE_BUTTON_PRESSED) {
                widget->data[0] = get_string_position(titlefont,
                    (char*)(widget->data + 1), event.data[1] - widget->x - 2);
                gui_repaint_window(window);
            }
            if (event.type == EVENT_KEYBOARD_BUTTON_PRESSED) {
                if (((char)event.data[0]) == '\b') {
                    if (widget->data[0] > 0) {
                        memmove(((char*)widget->data) + 3 + widget->data[0],
                            ((char*)widget->data) + 4 + widget->data[0],
                            strlen((char*)(widget->data + 1)) - widget->data[0] + 1);
                        widget->data[0]--;
                        gui_repaint_window(window);
                    }
                } else {
                    char *widgetdata = malloc(strlen((char*)(widget->data + 1)) + 6);
                    memcpy(widgetdata, widget->data, 4 + widget->data[0]);
                    widgetdata[4 + widget->data[0]] = (char)event.data[0];
                    memcpy(widgetdata + 5 + widget->data[0],
                        ((char*)widget->data) + 4 + widget->data[0],
                        strlen((char*)(widget->data + 1)) + 1 - widget->data[0]);
                    free(widget->data);
                    widget->data = (dword*) widgetdata;
                    gui_repaint_window(window);
                    widget->data[0]++;
                }
            }
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
                    if (focus != widget) {
                        focus = widget;
                        gui_repaint_window(window);
                        printf("Fokus: %d\n", widget->id);
                    }
                    widget_process_event(window, widget, event);
                    return 1;
                }
            }
            focus = 0;
            break;
        case EVENT_MOUSE_BUTTON_RELEASED:
            for (i = 0; i < list_size(window->widgets); i++) {
                widget_t *widget = (widget_t*)list_get_element_at(window->widgets, i);
                if (widget_is_at_position(widget, event.data[1], event.data[2])) {
                    if (focus != widget) {
                        focus = widget;
                        gui_repaint_window(window);
                        printf("Fokus: %d\n", widget->id);
                    }
                    widget_process_event(window, widget, event);
                    return 1;
                }
            }
            focus = 0;
            break;
        case EVENT_KEYBOARD_BUTTON_PRESSED:
            if (focus) {
                widget_process_event(window, focus, event);
                return 1;
            }
            return 0;
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
                dword text_width = get_text_width(titlefont, (char*)(widget->data + 1));
                render_text(window->bitmap, titlefont, (char*)(widget->data + 1),
                    widget->x + widget->width / 2 - text_width / 2,
                    widget->y + widget->height / 2 + 5);
                //TODO: Text zentrieren
                break;
            case GUI_WIDGET_EDITBOX:
                bitmap_draw_rect(window->bitmap, 0x000000, widget->x, widget->y,
                    widget->width, widget->height);
                bitmap_draw_rect(window->bitmap, 0xFFFFFF, widget->x + 1, widget->y + 1,
                    widget->width - 2, widget->height - 2);
                render_text(window->bitmap, titlefont, (char*)(widget->data + 1),
                    widget->x + 2, widget->y + 12);
                //TODO: Clipping bei zu langem Text
                if (widget == focus) {
                    int cursorpos = widget->data[0];
                    char tmplabel[cursorpos + 1];
                    tmplabel[cursorpos] = 0;
                    memcpy(tmplabel, (char*)(widget->data + 1), cursorpos);
                    cursorpos = get_text_width(titlefont, tmplabel);
                    render_text(window->bitmap, titlefont, "_",
                        widget->x + 2 + cursorpos, widget->y + 12);
                }
                break;
            case GUI_WIDGET_LABEL:
                break;
        }
    }
}
void destroy_widgets(window_t *window)
{
    //TODO
}
