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

#include "gui/gui.h"
#include "gui/commands.h"
#include "gui/widgets.h"
#include "video/bitmap.h"

#include "types.h"
#include "stdio.h"
#include "rpc.h"
#include "init.h"
#include "stdlib.h"

FILE *gui_control = 0;
int gui_command_data[10];

list_t *windowlist = 0;

byte close_button[9][9] =
{{0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0},
 {0,1,1,0,0,0,1,1,0},
 {0,0,1,1,0,1,1,0,0},
 {0,0,0,1,1,1,0,0,0},
 {0,0,0,1,1,1,0,0,0},
 {0,0,1,1,0,1,1,0,0},
 {0,1,1,0,0,0,1,1,0},
 {0,0,0,0,0,0,0,0,0}};

byte max_button[9][9] =
{{0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0},
 {0,1,1,1,1,1,1,1,0},
 {0,1,1,1,1,1,1,1,0},
 {0,1,0,0,0,0,0,1,0},
 {0,1,0,0,0,0,0,1,0},
 {0,1,0,0,0,0,0,1,0},
 {0,1,1,1,1,1,1,1,0},
 {0,0,0,0,0,0,0,0,0}};

byte min_button[9][9] =
{{0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0},
 {0,1,1,1,1,1,1,1,0},
 {0,1,1,1,1,1,1,1,0},
 {0,0,0,0,0,0,0,0,0}};

//Callback
void rpc_gui_callback(pid_t pid,
                      dword correlation_id,
                      size_t data_size,
                      void* data);

void (*gui_callback)(event_t) = 0;

int init_gui(void)
{
    //Suche GUI
    qword timeout = get_tick_count() + 5000000;
    while (init_service_get("gui1") == 0) {
        yield();
        if(get_tick_count() > timeout) {
            puts("[ LIBGUI ] Konnte GUI nicht finden!");
            return 0;
        }
    }
    
    //Öffne GUI Interface
    char tmp[30];
    sprintf(tmp, "%s:/control", "gui1");
    gui_control = fopen(tmp, "w");
    if (!gui_control) {
        printf("[ LIBGUI ] Konnte GUI nicht oeffnen.\n");
        return 0;
    } else {
        //Callback
        register_message_handler("LIBGUICB", rpc_gui_callback);
        
        gui_command_data[0] = GUI_SET_CALLBACK_FUNCTION;
        memcpy(&gui_command_data[1], "LIBGUICB", 9);
        if (!rpc_get_dword(gui_control->pid, "GUIFNCTS", sizeof(int) * 4,
            (char*)gui_command_data)) {
            fclose(gui_control);
            gui_control = 0;
            printf("[ LIBGUI ] Konnte Callback nicht initialisieren.\n");
            return 0;
        }
    }
    
    windowlist = list_create();
    return 1;
}
int close_gui(void)
{
    int i;
    for (i = 0; i < list_size(windowlist); i++) {
        free(list_get_element_at(windowlist, i));
    }
    if (!gui_control) {
        return 0;
    }
    fclose(gui_control);
    return 1;
}

window_t *gui_create_window(int x, int y, int width, int height, dword flags)
{
    window_t *newwindow = malloc(sizeof(window_t));
    memset(newwindow, 0, sizeof(window_t));
    gui_command_data[0] = GUI_CREATE_WINDOW;
    gui_command_data[1] = x;
    gui_command_data[2] = y;
    gui_command_data[3] = width;
    gui_command_data[4] = height;
    gui_command_data[5] = flags;
    response_t *response = rpc_get_response(gui_control->pid, "GUIFNCTS",
        sizeof(int) * 6, (char*)gui_command_data);
    if (response->data_length <= 4) {
        free(newwindow);
        return 0;
    }
    dword *responsedata = response->data;
    newwindow->x = responsedata[0];
    newwindow->y = responsedata[1];
    newwindow->width = responsedata[2];
    newwindow->height = responsedata[3];
    newwindow->flags = responsedata[4];
    newwindow->id = responsedata[5];
    newwindow->shmid = responsedata[6];
    newwindow->bitmap.width = responsedata[7];
    newwindow->bitmap.height = responsedata[8];
    newwindow->bitmap.bpp = responsedata[9];
    newwindow->bitmap.data = open_shared_memory(newwindow->shmid);
    newwindow->widgets = list_create();
    list_push(windowlist, newwindow);
    return newwindow;
}
void gui_delete_window(window_t *window)
{
    close_shared_memory(window->shmid);
    gui_command_data[0] = GUI_CLOSE_WINDOW;
    gui_command_data[1] = window->id;
    rpc_get_dword(gui_control->pid, "GUIFNCTS", sizeof(int) * 2,
        (char*)gui_command_data);
    int i;
    for (i = 0; i < list_size(windowlist); i++) {
        if (list_get_element_at(windowlist, i) == window) {
            //TODO: Widgets aufräumen
            free(list_get_element_at(windowlist, i));
            list_remove(windowlist, i);
            break;
        }
    }
}

void gui_repaint_window(window_t *window)
{
    gui_command_data[0] = GUI_REPAINT_WINDOW;
    gui_command_data[1] = window->id;
    rpc_get_dword(gui_control->pid, "GUIFNCTS", sizeof(int) * 2,
        (char*)gui_command_data);
}

void gui_start_moving_window(window_t *window)
{
    gui_command_data[0] = GUI_MOVE_WINDOW;
    gui_command_data[1] = window->id;
    rpc_get_dword(gui_control->pid, "GUIFNCTS", sizeof(int) * 2,
        (char*)gui_command_data);
}
void gui_start_resizing_window(window_t *window, dword position)
{
    gui_command_data[0] = GUI_RESIZE_WINDOW;
    gui_command_data[1] = window->id;
    gui_command_data[2] = position;
    rpc_get_dword(gui_control->pid, "GUIFNCTS", sizeof(int) * 3,
        (char*)gui_command_data);
}

void gui_send_event(event_t event)
{
    if (gui_callback) gui_callback(event);
}

void gui_set_callback(void (*cb)(event_t))
{
    gui_callback = cb;
}

void rpc_gui_callback(pid_t pid,
                      dword correlation_id,
                      size_t data_size,
                      void* data)
{
    //printf("[ LIBGUI ] Callback!\n");
    event_t event = *((event_t*)data);
    //Suche Fenster
    window_t *window = 0;
    int i;
    for (i = 0; i < list_size(windowlist); i++) {
        window_t *tmpwindow = (window_t*)list_get_element_at(windowlist, i);
        if (tmpwindow->id == event.windowid) {
            window = tmpwindow;
            break;
        }
    }
    //Ist nicht für uns?
    if (!window) {
        printf("[ LIBGUI ] Fehlerhafte Fenster-ID empfangen!\n");
        return;
    }
    //Event evtl selber benutzen
    if (event.type == EVENT_WINDOW_RESIZED) {
        window->x = event.data[0];
        window->y = event.data[1];
        window->width = event.data[2];
        window->height = event.data[3];
        rpc_send_dword_response(pid, correlation_id, 1);
    } else if (event.type == EVENT_MOUSE_BUTTON_PRESSED) {
        if (event.data[2] < 11) {
            gui_start_moving_window(window);
            rpc_send_dword_response(pid, correlation_id, 1);
            return;
        } else {
            rpc_send_dword_response(pid, correlation_id, 1);
            if (!process_event(window, event)) {
                if (gui_callback) gui_callback(*((event_t*)data));
            }
        }
    } else if (event.type == EVENT_WINDOW_PAINT) {
        //Fenster leeren
        //TODO
    } else {
        rpc_send_dword_response(pid, correlation_id, 1);
        if (!process_event(window, event)) {
            if (gui_callback) gui_callback(*((event_t*)data));
        }
    }
    if (event.type == EVENT_WINDOW_PAINT) {
        if (gui_callback) gui_callback(*((event_t*)data));
        //Zeichne Widgets
        draw_widgets(window);
        //Zeichne Rahmen
        if (window->flags & GUI_WINDOW_FRAME) {
            bitmap_draw_rect(window->bitmap, 0xAAAAAA, 0, 0, window->width, 1);
            bitmap_draw_rect(window->bitmap, 0xAAAAAA, 0, 10, window->width, 1);
            bitmap_draw_rect(window->bitmap, 0xAAAAAA, 0, window->height - 1, window->width, 1);
            bitmap_draw_rect(window->bitmap, 0xAAAAAA, 0, 0, 1, window->height);
            bitmap_draw_rect(window->bitmap, 0xAAAAAA, window->width - 1, 0, 1, window->height);
            bitmap_draw_rect(window->bitmap, 0xFFFFFF, 1, 1, window->width - 2, 9);
            
            if (window->flags & GUI_WINDOW_CLOSE) {
                //TODO: Bitmap benutzen
                int x, y;
                for (y = 0; y < 9; y++) {
                    for (x = 0; x < 9; x++) {
                        if (close_button[y][x]) {
                            bitmap_draw_rect(window->bitmap, 0,
                                window->width - 11 + x, 1 + y, 1, 1);
                        }
                    }
                }
            }
            if (window->flags & GUI_WINDOW_MAXIMIZE) {
                //TODO: Bitmap benutzen
                int x, y;
                for (y = 0; y < 9; y++) {
                    for (x = 0; x < 9; x++) {
                        if (max_button[y][x]) {
                            bitmap_draw_rect(window->bitmap, 0,
                                window->width - 21 + x, 1 + y, 1, 1);
                        }
                    }
                }
            }
            if (window->flags & GUI_WINDOW_MINIMIZE) {
                //TODO: Bitmap benutzen
                int x, y;
                for (y = 0; y < 9; y++) {
                    for (x = 0; x < 9; x++) {
                        if (min_button[y][x]) {
                            bitmap_draw_rect(window->bitmap, 0,
                                window->width - 31 + x, 1 + y, 1, 1);
                        }
                    }
                }
            }
        }
        //Sende Antwort erst später, damit das Fenster korrekt gerendert wird
        rpc_send_dword_response(pid, correlation_id, 1);
    }
}
