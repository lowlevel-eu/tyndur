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
 
#include "button.h"

#include "video/video.h"
#include "bitmap.h"
#include "widget.h"

#include "stdlib.h"

widget_class_t button_class =
{
    "button",
    button_create,
    button_draw,
    button_inject_event,
    button_set_label
};

void button_create(widget_t button)
{
    button->data = malloc(sizeof(button_data_t));
    bitmap_t text = render_string(default_font, " ");
    ((button_data_t*)button->data)->textbitmap = text;
    ((button_data_t*)button->data)->label = strdup(" ");
    ((button_data_t*)button->data)->text = video_load_bitmap(text.width, text.height, text.data);
    ((button_data_t*)button->data)->state = 0;
}
void button_draw(widget_t button)
{
    bitmap_t text = ((button_data_t*)button->data)->textbitmap;
    if (button->parent){
        int x = ((widget_t)button->parent)->position.x + button->position.x;
        int y = ((widget_t)button->parent)->position.y + button->position.y;
        video_draw_rect(x, y, button->size.width, 
            button->size.height, 0xFF000000);
        video_draw_rect(x + 1, y + 1, button->size.width - 2,
            button->size.height - 2, 0xFFFFFFFF);
        if (((button_data_t*)button->data)->state == 2) {
            video_draw_rect(x + 2, y + 2, 1,
                button->size.height - 4, 0xFF000000);
            video_draw_rect(x + ((widget_data_t*)button)->size.width - 3, y + 2, 1,
                button->size.height - 4, 0xFF000000);
        }
        if (((button_data_t*)button->data)->text) {
            video_draw_bitmap(((button_data_t*)button->data)->text, 
                x + ((widget_data_t*)button)->size.width / 2 - text.width / 2,
                y + button->size.height / 2 - text.height / 2, 0);
        }
    } else {
        video_draw_rect(button->position.x, button->position.y,
            button->size.width, button->size.height, 0xFF000000);
        video_draw_rect(button->position.x + 1, button->position.y + 1,
            button->size.width - 2, button->size.height - 2, 0xFFFFFFFF);
        if (((button_data_t*)button->data)->text) {
            video_draw_bitmap(((button_data_t*)button->data)->text,
                button->position.x + button->size.width / 2 - text.width / 2,
                button->position.y + button->size.height / 2 - text.height / 2,
                0);
        }
    }
}
void button_inject_event(widget_t button, event_t event)
{
    //printf("Button:\n");
    switch (event.type)
    {
        case EVENT_MOUSE_ENTERED_AREA:
            ((button_data_t*)button->data)->state = 1;
            break;
        case EVENT_MOUSE_LEFT_AREA:
            ((button_data_t*)button->data)->state = 0;
            break;
        case EVENT_MOUSE_BUTTON_PRESSED:
            ((button_data_t*)button->data)->state = 2;
            break;
        case EVENT_MOUSE_BUTTON_RELEASED:
            if (((button_data_t*)button->data)->state == 2) {
                ((button_data_t*)button->data)->state = 1;
                event_t newevent;
                newevent.sender = button;
                newevent.type = EVENT_BUTTON_CLICKED;
                send_gui_event(button->owner, newevent);
            }
            break;
    }
    //printf("state = %d\n", ((button_data_t*)button->data)->state);
}

void button_set_label(widget_t button, char *label)
{
    video_delete_bitmap(((button_data_t*)button->data)->text);
    free(((button_data_t*)button->data)->textbitmap.data);
    free(((button_data_t*)button->data)->label);
    ((button_data_t*)button->data)->label = strdup(label);
    bitmap_t text = render_string(default_font, label);
    ((button_data_t*)button->data)->textbitmap = text;
    ((button_data_t*)button->data)->text = video_load_bitmap(text.width, text.height, text.data);
}
