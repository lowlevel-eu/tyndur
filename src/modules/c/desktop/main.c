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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
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

#include "types.h"
#include "stdio.h"
#include "lostio.h"
#include "gui/gui.h"
#include "gui/widgets.h"

#include "video/bitmap.h"

#define _USE_START_
#include "init.h"

void guicb(event_t event);

window_t *desktop = 0;
window_t *panel = 0;
window_t *test1 = 0;
window_t *test2 = 0;
widget_t *button1 = 0;
widget_t *editbox1 = 0;

widget_t *calc = 0;

int main(int argc, char* argv[])
{
    init_gui();
    gui_set_callback(guicb);
    desktop = gui_create_window(0, 0, 640, 480, GUI_WINDOW_ALWAYS_ON_BOTTOM);
    gui_repaint_window(desktop);
    panel = gui_create_window(0, 460, 640, 20, GUI_WINDOW_ALWAYS_ON_TOP);
    calc = create_button(panel, 10, 2, 100, 16, "Taschenrechner");
    gui_repaint_window(panel);
    test1 = gui_create_window(10, 10, 300, 200, GUI_WINDOW_FRAME | GUI_WINDOW_MAXIMIZE | GUI_WINDOW_MINIMIZE);
    button1 = create_button(test1, 10, 20, 100, 30, "Button");
    editbox1 = create_edit_box(test1, 10, 60, 100, 20, "Editbox1");
    gui_set_window_title(test1, "Testfenster Nr. 1");
    test2 = gui_create_window(30, 30, 300, 200, GUI_WINDOW_FRAME | GUI_WINDOW_CLOSE);
    gui_set_window_title(test2, "Testfenster Nr. 2");
    gui_repaint_window(test2);
    
    
    while(TRUE) {
        yield();
    }
}

void guicb(event_t event)
{
    //printf("Event, Typ: %d\n", event.type);
    switch (event.type)
    {
        case EVENT_WINDOW_PAINT:
            //printf("Paint-Event (%d)!\n", event.windowid);
            if (event.windowid == desktop->id) {
                bitmap_draw_rect(desktop->bitmap, 0xFFFFFF, 0, 0, desktop->width, desktop->height);
                bitmap_draw_rect(desktop->bitmap, 0xFF0000, 10, 10, 100, 100);
                bitmap_draw_rect(desktop->bitmap, 0x00FF00, 100, 100, 100, 100);
                bitmap_draw_rect(desktop->bitmap, 0x0000FF, 190, 190, 100, 100);
            } else if (event.windowid == test1->id) {
                bitmap_draw_rect(test1->bitmap, 0x00FF00, 0, 0, test1->width, test1->height);
            } else if (event.windowid == test2->id) {
                bitmap_draw_rect(test2->bitmap, 0x0000FF, 0, 0, test2->width, test2->height);
            } else if (event.windowid == panel->id) {
                bitmap_draw_rect(panel->bitmap, 0x999999, 0, 0, panel->width, panel->height);
            }
            break;
        case EVENT_MOUSE_BUTTON_PRESSED:
            if (event.windowid == test1->id) {
                //gui_start_moving_window(test1);
            }
            if (event.windowid == test2->id) {
                //gui_start_resizing_window(test2, 4);
            }
            break;
        case EVENT_BUTTON_PRESSED:
            printf("Button %d wurde angeklickt.\n", event.data[0]);
            if (event.data[0] == calc->id) {
                pid_t pid = init_execute("floppy:/devices/fd0|fat:/apps/calc");
            }
            break;
        case EVENT_CLOSE_WINDOW:
            if (event.windowid == test2->id) {
                gui_delete_window(test2);
                test2 = 0;
            }
            break;
    }
}
