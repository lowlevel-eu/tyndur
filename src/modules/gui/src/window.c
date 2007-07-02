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

#include "window.h"

#include "video/video.h"
#include "widget.h"

#include "stdlib.h"

widget_class_t window_class =
{
    "window",
    window_create,
    window_draw,
    window_inject_event,
    window_set_label
};

void window_create(widget_t window)
{
    window->data = malloc(sizeof(window_data_t));
    bitmap_t text = render_string(default_font, " ");
    ((window_data_t*)window->data)->textbitmap = text;
    ((window_data_t*)window->data)->label = strdup(" ");
    ((window_data_t*)window->data)->text = video_load_bitmap(text.width, text.height, text.data);
    ((window_data_t*)window->data)->state = 0;
}

void window_draw(widget_t window)
{
    window_data_t *data = (window_data_t*)window->data;
    if (window->parent){
        widget_t parent = (widget_t)window->parent;
        int x = parent->position.x + window->position.x;
        int y = parent->position.y + window->position.y;
        video_draw_rect(x, y, window->size.width, window->size.height,
            0xFFFFFFFF);
        
        video_draw_rect(x, y, window->size.width, 1, 0xFF000000);
        video_draw_rect(x, y + 15, window->size.width, 1, 0xFF000000);
        video_draw_rect(x, y + window->size.height - 1, window->size.width, 1,
            0xFF000000);
        
        video_draw_rect(x, y, 1, window->size.height, 0xFF000000);
        video_draw_rect(x + window->size.width - 1, y, 1, window->size.height,
            0xFF000000);
        if (data->text) {
            video_draw_bitmap(data->text, x + 3, y + 1, 0);
        }
    } else {
        video_draw_rect(window->position.x, window->position.y,
            window->size.width, window->size.height, 0xFFFFFFFF);
        
        video_draw_rect(window->position.x, window->position.y,
            window->size.width, 1, 0xFF000000);
        video_draw_rect(window->position.x, window->position.y + 15,
            window->size.width, 1, 0xFF000000);
        video_draw_rect(window->position.x,
            window->position.y + window->size.height - 1, window->size.width, 1,
            0xFF000000);
        
        video_draw_rect(window->position.x, window->position.y, 1,
            window->size.height, 0xFF000000);
        video_draw_rect(window->position.x + window->size.width - 1,
            window->position.y, 1, window->size.height, 0xFF000000);
        if (data->text) {
            video_draw_bitmap(data->text,
                                window->position.x + 3,
                                window->position.y + 1,
                                0);
        }
    }
}

void window_inject_event(widget_t window, event_t event)
{
    //printf("Window!\n");
    switch (event.type)
    {
        case EVENT_MOUSE_BUTTON_PRESSED:
            start_dragging(window);
            break;
    }
}
void window_set_label(widget_t window, char *label)
{
    window_data_t *data = (window_data_t*)window->data;
    video_delete_bitmap(data->text);
    free(data->textbitmap.data);
    free(data->label);
    data->label = strdup(label);
    bitmap_t text = render_string(default_font, label);
    data->textbitmap = text;
    data->text = video_load_bitmap(text.width, text.height, text.data);
}
