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
#include "textbox.h"
#include "widget.h"
#include "video/video.h"

#include "stdlib.h"
#include "string.h"
#include "stdio.h"

widget_class_t textbox_class =
{
    "textbox",
    textbox_create,
    textbox_draw,
    textbox_inject_event,
    textbox_set_label
};

void textbox_create(widget_t textbox)
{
    textbox->data = malloc(sizeof(textbox_data_t));
    ((textbox_data_t*)textbox->data)->text = 0;
    ((textbox_data_t*)textbox->data)->bitmap.data = 0;
    ((textbox_data_t*)textbox->data)->handle = 0;
}
void textbox_draw(widget_t textbox)
{
    widget_t parent = (widget_t)textbox->parent;
    textbox_data_t *data = (textbox_data_t*)textbox->data;
    if (data->text && data->handle) {
        int x = parent->position.x + textbox->position.x;
        int y = parent->position.y + textbox->position.y;
        video_draw_bitmap(data->handle, x, y, 0);
    }
}
void textbox_inject_event(widget_t textbox, event_t event)
{
}
void textbox_set_label(widget_t textbox, char *label)
{
    int i;
    textbox_data_t *data = (textbox_data_t*)textbox->data;
    //Aufräumen
    if (data->text) {
        free(data->text);
        data->text = 0;
    }
    if (data->bitmap.data) {
        free(data->bitmap.data);
        data->bitmap.data = 0;
    }
    if (data->handle) {
         video_delete_bitmap(data->handle);
    }
    data->handle = 0;
    
    if (label) {
        data->text = strdup(label);
        //Zeilen in Bitmaps rendern
        char *tmp = data->text;
        data->bitmap = render_string(default_font, tmp);
        if (!data->bitmap.data) {
            printf("Kein Speicher mehr?\n");
            free(data->text);
            data->text = 0;
            return;
        }
        data->handle = video_load_bitmap(data->bitmap.width,
            data->bitmap.height, data->bitmap.data);
    }
    //printf("[ GUI ] Textbox gefuellt.\n");
}
