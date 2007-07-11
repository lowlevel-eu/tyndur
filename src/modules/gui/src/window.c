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

#include "window.h"
#include "video/bitmap.h"

#include "stdlib.h"
#include "collections.h"
#include "syscall.h"
#include "rpc.h"

//Alle Fenster
list_t *windowlist = 0;

//Fenster, aufgeteilt in Bereiche (Hintergrund, Normal, Vordergrund)
list_t *windowlist_bottom = 0;
list_t *windowlist_normal = 0;
list_t *windowlist_top = 0;

list_t *eventreceivers = 0;

int lastid = 0;

dword moving_window = 0;
dword resizing_window = 0;
dword resizing_position = 0;
int resizing_x = 0;
int resizing_y = 0;
int resizing_width = 0;
int resizing_height = 0;

//Variablen aus main.c:
extern int screen_width;
extern int screen_height;
extern int screen_bpp;

extern bitmap_t screen;

extern int need_update;

extern int mousex;
extern int mousey;

//Fenstersystem initialisieren
void init_windows(void)
{
    windowlist = list_create();
    eventreceivers = list_create();
    windowlist_bottom = list_create();
    windowlist_normal = list_create();
    windowlist_top = list_create();
}

//Fenster erstellen
server_window_t *create_window(pid_t pid, int x, int y, dword width, dword height, dword flags)
{
    int id = lastid++;
    //Speicher für Fenster reservieren und mit Werten füllen
    server_window_t *newwindow = malloc(sizeof(server_window_t));
    memset(newwindow, 0, sizeof(server_window_t));
    newwindow->id = id;
    newwindow->shmid = create_shared_memory(screen_width * screen_height * screen_bpp / 8);
    newwindow->x = x;
    newwindow->y = y;
    newwindow->width = width;
    newwindow->height = height;
    newwindow->owner = pid;
    newwindow->flags = flags;
    newwindow->bitmap.width = screen_width;
    newwindow->bitmap.height = screen_height;
    newwindow->bitmap.bpp = screen_bpp;
    newwindow->bitmap.data = open_shared_memory(newwindow->shmid);
    
    //Einfügen in die Fensterlisten
    list_push(windowlist, newwindow);
    if (flags & GUI_WINDOW_ALWAYS_ON_BOTTOM) {
        list_push(windowlist_bottom, newwindow);
    } else if (flags & GUI_WINDOW_ALWAYS_ON_TOP) {
        list_push(windowlist_top, newwindow);
    } else {
        list_push(windowlist_normal, newwindow);
    }
    
    need_update = 1;
    newwindow->needs_repainting = 1;
    return newwindow;
}
//Fenster komplett löschen
dword delete_window(pid_t pid, dword windowid)
{
    int i, j;
    for (i = list_size(windowlist) - 1; i >= 0; i--) {
        server_window_t *window = (server_window_t *)list_get_element_at(windowlist, i);
        if (window->id == windowid) {
            if (window->flags & GUI_WINDOW_ALWAYS_ON_BOTTOM) {
                for (j = list_size(windowlist_bottom) - 1; j >= 0; j--) {
                    server_window_t *window = (server_window_t *)list_get_element_at(windowlist_bottom, j);
                    if (window->id == windowid) {
                        list_remove(windowlist_bottom, j);
                        break;
                    }
                }
            } else if (window->flags & GUI_WINDOW_ALWAYS_ON_TOP) {
                for (j = list_size(windowlist_top) - 1; j >= 0; j--) {
                    server_window_t *window = (server_window_t *)list_get_element_at(windowlist_top, j);
                    if (window->id == windowid) {
                        list_remove(windowlist_top, j);
                        break;
                    }
                }
            } else {
                for (j = list_size(windowlist_normal) - 1; j >= 0; j--) {
                    server_window_t *window = (server_window_t *)list_get_element_at(windowlist_normal, j);
                    if (window->id == windowid) {
                        list_remove(windowlist_normal, j);
                        break;
                    }
                }
            }
            close_shared_memory(window->shmid);
            list_remove(windowlist, i);
            free(window);
            need_update = 1;
            //printf("[ GUI ] Fenster geloescht.\n");
            return 1;
        }
    }
    return 0;
}
//Fenster neu zeichnen lassen
dword redraw_window(dword windowid)
{
    int i;
    for (i = list_size(windowlist) - 1; i >= 0; i--) {
        server_window_t *window = (server_window_t *)list_get_element_at(windowlist, i);
        if (window->id == windowid) {
            window->needs_repainting = 1;
            need_update = 1;
            return 1;
        }
    }
    return 0;
}
//Alle Fenster auf den Videobuffer zeichnen
void draw_all_windows(void)
{
    int i;
    for (i = list_size(windowlist_bottom) - 1; i >= 0; i--) {
        server_window_t *window = (server_window_t *)list_get_element_at(windowlist_bottom, i);
        //Zeichne benötigte Fenster neu
        if (window->needs_repainting) {
            event_t event;
            event.windowid = window->id;
            event.type = EVENT_WINDOW_PAINT;
            send_event(window->owner, event);
            window->needs_repainting = 0;
        }
        //Zeichne Fenster auf den Buffer
        bitmap_draw_bitmap(screen, window->bitmap, window->x, window->y,
            window->width, window->height);
    }
    for (i = list_size(windowlist_normal) - 1; i >= 0; i--) {
        server_window_t *window = (server_window_t *)list_get_element_at(windowlist_normal, i);
        //Zeichne benötigte Fenster neu
        if (window->needs_repainting) {
            event_t event;
            event.windowid = window->id;
            event.type = EVENT_WINDOW_PAINT;
            send_event(window->owner, event);
            window->needs_repainting = 0;
        }
        //Zeichne Fenster auf den Buffer
        bitmap_draw_bitmap(screen, window->bitmap, window->x, window->y,
            window->width, window->height);
    }
    for (i = list_size(windowlist_top) - 1; i >= 0; i--) {
        server_window_t *window = (server_window_t *)list_get_element_at(windowlist_top, i);
        //Zeichne benötigte Fenster neu
        if (window->needs_repainting) {
            event_t event;
            event.windowid = window->id;
            event.type = EVENT_WINDOW_PAINT;
            send_event(window->owner, event);
            window->needs_repainting = 0;
        }
        //Zeichne Fenster auf den Buffer
        bitmap_draw_bitmap(screen, window->bitmap, window->x, window->y,
            window->width, window->height);
    }
    if (resizing_window) {
        bitmap_draw_rect(screen, 0, resizing_x, resizing_y, resizing_width, 1);
        bitmap_draw_rect(screen, 0, resizing_x, resizing_y + resizing_height - 1, resizing_width, 1);
        bitmap_draw_rect(screen, 0, resizing_x, resizing_y, 1, resizing_height);
        bitmap_draw_rect(screen, 0, resizing_x + resizing_width - 1, resizing_y, 1, resizing_height);
    }
}

void set_event_receiver(pid_t pid, char *name)
{
    //Vorhandenen Receiver ändern
    int i;
    for (i = 0; i < list_size(eventreceivers); i++) {
        if (((event_receiver_t*)list_get_element_at(windowlist, i))->owner == pid) {
            strncpy(((event_receiver_t*)list_get_element_at(windowlist, i))->name,
                name, 10);
            return;
        }
    }
    //Kein Receiver vorhanden, neuen erstellen
    event_receiver_t *newreceiver = malloc(sizeof(event_receiver_t));
    newreceiver->owner = pid;
    strncpy(newreceiver->name, name, 10);
    list_push(eventreceivers, newreceiver);
}

int start_moving_window(dword windowid)
{
    if (resizing_window) {
        stop_resizing_window();
    }
    moving_window = windowid;
    return 1;
}
int start_resizing_window(dword windowid, dword position)
{
    if (moving_window) {
        moving_window = 0;
    }
    int i;
    for (i = list_size(windowlist) - 1; i >= 0; i--) {
        server_window_t *window = (server_window_t *)list_get_element_at(windowlist, i);
        if (window->id == windowid) {
            resizing_window = windowid;
            resizing_position = position;
            resizing_x = window->x;
            resizing_y = window->y;
            resizing_width = window->width;
            resizing_height = window->height;
            return 1;
        }
    }
    return 0;
}
void stop_resizing_window(void)
{
    int i;
    for (i = list_size(windowlist) - 1; i >= 0; i--) {
        server_window_t *window = (server_window_t *)list_get_element_at(windowlist, i);
        if (window->id == resizing_window) {
            window->width = resizing_width;
            window->height = resizing_height;
            event_t event;
            event.windowid = window->id;
            event.type = EVENT_WINDOW_RESIZED;
            event.data[0] = window->x;
            event.data[1] = window->y;
            event.data[2] = window->width;
            event.data[3] = window->height;
            send_event(window->owner, event);
            window->needs_repainting = 1;
            break;
        }
    }
    resizing_window = 0;
    resizing_position = 0;
    resizing_x = 0;
    resizing_y = 0;
    resizing_width = 0;
    resizing_height = 0;
}

server_window_t *get_window_at_position(int x, int y)
{
    int i;
    for (i = 0; i < list_size(windowlist_top); i++) {
        server_window_t *window = (server_window_t *)list_get_element_at(windowlist_top, i);
        if ((window->x <= x) && (window->x + window->width > x)
            && (window->y <= y) && (window->y + window->height > y)) {
            return window;
        }
    }
    for (i = 0; i < list_size(windowlist_normal); i++) {
        server_window_t *window = (server_window_t *)list_get_element_at(windowlist_normal, i);
        if ((window->x <= x) && (window->x + window->width > x)
            && (window->y <= y) && (window->y + window->height > y)) {
            return window;
        }
    }
    for (i = 0; i < list_size(windowlist_bottom); i++) {
        server_window_t *window = (server_window_t *)list_get_element_at(windowlist_bottom, i);
        if ((window->x <= x) && (window->x + window->width > x)
            && (window->y <= y) && (window->y + window->height > y)) {
            return window;
        }
    }
    return 0;
}

void send_event(pid_t pid, event_t event)
{
    int i;
    for (i = 0; i < list_size(eventreceivers); i++) {
        if (((event_receiver_t*)list_get_element_at(eventreceivers, i))->owner == pid) {
            //printf("Sende Event (%d:\"%s\").\n", pid, ((event_receiver_t*)list_get_element_at(eventreceivers, i))->name);
            rpc_get_dword(pid,
                ((event_receiver_t*)list_get_element_at(eventreceivers, i))->name,
                sizeof(event_t), (char*)&event);
        }
    }
}

void inject_mouse_move(int x, int y)
{
    if (moving_window) {
        int i;
        for (i = list_size(windowlist) - 1; i >= 0; i--) {
            server_window_t *window = (server_window_t *)list_get_element_at(windowlist, i);
            if (window->id == moving_window) {
                window->x += x;
                window->y += y;
                break;
            }
        }
        need_update = 1;
    } else if (resizing_window) {
        switch (resizing_position)
        {
            case 0:
                //TODO
                break;
            case 1:
                //TODO
                break;
            case 2:
                //TODO
                break;
            case 3:
                resizing_width += x;
                if (resizing_width > screen_width) {
                    resizing_width = screen_width;
                }
                if (resizing_width < 20) {
                    resizing_width = 20;
                }
                break;
            case 4:
                resizing_width += x;
                if (resizing_width > screen_width) {
                    resizing_width = screen_width;
                }
                if (resizing_width < 20) {
                    resizing_width = 20;
                }
                resizing_height += y;
                if (resizing_height > screen_height) {
                    resizing_height = screen_height;
                }
                if (resizing_height < 20) {
                    resizing_height = 20;
                }
                break;
            case 5:
                 resizing_height += y;
                if (resizing_height > screen_height) {
                    resizing_height = screen_height;
                }
                if (resizing_height < 20) {
                    resizing_height = 20;
                }
                break;
            case 6:
                //TODO
                break;
            case 7:
                //TODO
                break;
        }
        need_update = 1;
    } else {
        server_window_t *oldposition = get_window_at_position(mousex - x, mousey - y);
        server_window_t *newposition = get_window_at_position(mousex, mousey);
        if (!oldposition && !newposition) {
            //printf("Keine Fenster an Mausposition!\n");
            return;
        }
        if (oldposition == newposition) {
            event_t event;
            event.windowid = oldposition->id;
            event.type = EVENT_MOUSE_MOVED;
            event.data[0] = x;
            event.data[1] = y;
            event.data[2] = mousex - oldposition->x;
            event.data[3] = mousey - oldposition->y;
            //printf("Sende EVENT_MOUSE_MOVED!\n");
            send_event(oldposition->owner, event);
        } else {
            if (oldposition) {
                event_t event;
                event.windowid = oldposition->id;
                event.type = EVENT_MOUSE_LEFT_AREA;
                event.data[0] = x;
                event.data[1] = y;
                event.data[2] = mousex - oldposition->x;
                event.data[3] = mousey - oldposition->y;
                //printf("Sende EVENT_MOUSE_LEFT_AREA!\n");
                send_event(oldposition->owner, event);
            }
            if (newposition) {
                event_t event;
                event.windowid = newposition->id;
                event.type = EVENT_MOUSE_ENTERED_AREA;
                event.data[0] = x;
                event.data[1] = y;
                event.data[2] = mousex - newposition->x;
                event.data[3] = mousey - newposition->y;
                //printf("Sende EVENT_MOUSE_ENTERED_AREA!\n");
                send_event(newposition->owner, event);
            }
        }
    }
}
void inject_mouse_button_pressed(int button)
{
    server_window_t *window = get_window_at_position(mousex, mousey);
    if (window) {
        int i;
        int window_changed = 0;
        //Verschiebe Fenster nach vorne
        for (i = list_size(windowlist) - 1; i >= 1; i--) {
            if ((server_window_t *)list_get_element_at(windowlist, i) == window) {
                list_remove(windowlist, i);
                list_push(windowlist, window);
                window_changed = 1;
                break;
            }
        }
        if (window->flags & GUI_WINDOW_ALWAYS_ON_BOTTOM) {
            for (i = list_size(windowlist_bottom) - 1; i >= 1; i--) {
                if ((server_window_t *)list_get_element_at(windowlist_bottom, i) == window) {
                    list_remove(windowlist_bottom, i);
                    list_push(windowlist_bottom, window);
                    window_changed = 1;
                    break;
                }
            }
        } else if (window->flags & GUI_WINDOW_ALWAYS_ON_TOP) {
            for (i = list_size(windowlist_top) - 1; i >= 1; i--) {
                if ((server_window_t *)list_get_element_at(windowlist_top, i) == window) {
                    list_remove(windowlist_top, i);
                    list_push(windowlist_top, window);
                    window_changed = 1;
                    break;
                }
            }
        } else {
            for (i = list_size(windowlist_normal) - 1; i >= 1; i--) {
                if ((server_window_t *)list_get_element_at(windowlist_normal, i) == window) {
                    list_remove(windowlist_normal, i);
                    list_push(windowlist_normal, window);
                    window_changed = 1;
                    break;
                }
            }
        }
        if (window_changed) need_update = 1;
        //Sende Event
        event_t event;
        event.windowid = window->id;
        event.type = EVENT_MOUSE_BUTTON_PRESSED;
        event.data[0] = button;
        event.data[1] = mousex - window->x;
        event.data[2] = mousey - window->y;
        send_event(window->owner, event);
    }
}
void inject_mouse_button_released(int button)
{
    if (moving_window) {
        moving_window = 0;
        need_update = 1;
    }
    if (resizing_window) {
        stop_resizing_window();
        need_update = 1;
    }
    server_window_t *window = get_window_at_position(mousex, mousey);
    if (window) {
        event_t event;
        event.windowid = window->id;
        event.type = EVENT_MOUSE_BUTTON_RELEASED;
        event.data[0] = button;
        event.data[1] = mousex - window->x;
        event.data[2] = mousey - window->y;
        send_event(window->owner, event);
    }
}
