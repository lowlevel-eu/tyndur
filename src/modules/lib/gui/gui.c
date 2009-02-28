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

#include "gui/gui.h"
#include "gui/commands.h"
#include "gui/widgets.h"
#include "video/bitmap.h"
#include "video/color.h"

#include "types.h"
#include "stdio.h"
#include "rpc.h"
#include "init.h"
#include "stdlib.h"
#include <syscall.h>

FILE *gui_control = 0;
int gui_command_data[10];

list_t *windowlist = 0;

font_t *titlefont = 0;

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
    qword timeout = get_tick_count() + 15000000;
    while (init_service_get("gui1") == 0) {
        yield();
        if(get_tick_count() > timeout) {
            puts("[ LIBGUI ] Konnte GUI nicht finden!");
            return 0;
        }
    }
    
    //÷ffne GUI Interface
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
    
    titlefont = gui_get_font("vera", 10);
    return 1;
}
int close_gui(void)
{
    while (list_size(windowlist) > 0) {
        gui_delete_window((window_t*)list_get_element_at(windowlist, 0));
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
    newwindow->title = 0;
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
    if (window->title) {
        free(window->title);
    }
    destroy_widgets(window);
    int i;
    for (i = 0; i < list_size(windowlist); i++) {
        if (list_get_element_at(windowlist, i) == window) {
            //TODO: Widgets aufr‰umen
            free(list_get_element_at(windowlist, i));
            list_remove(windowlist, i);
            break;
        }
    }
    printf("[ LIBGUI ] Fenster geloescht.\n");
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

void gui_set_window_title(window_t *window, const char *title)
{
    if (window->title) {
        free(window->title);
    }
    if (title) {
        window->title = strdup(title);
    } else {
        window->title = 0;
    }
    gui_repaint_window(window);
}
const char *gui_get_window_title(window_t *window)
{
    return window->title;
}

void gui_send_event(event_t event)
{
    if (gui_callback) gui_callback(event);
}

void gui_set_callback(void (*cb)(event_t))
{
    gui_callback = cb;
}

font_t *gui_get_font(const char *name, int size)
{
    int *cmddata = malloc(8 + strlen(name) + 1);
    cmddata[0] = GUI_GET_FONT;
    cmddata[1] = size;
    strcpy((char*)&cmddata[2], name);
    response_t *response = rpc_get_response(gui_control->pid, "GUIFNCTS",
        8 + strlen(name) + 1, (char*)cmddata);
    free(cmddata);
    if (response->data_length <= 4) {
        printf("[ LIBGUI ] Konnte Font nicht laden (\"%s\", %d).\n", name, size);
        return 0;
    }
    dword *fontdata = (dword*)response->data;
    font_t *font = malloc(sizeof(font_t));
    font->name = strdup(name);
    font->size = size;
    font->id = fontdata[0];
    font->shmid = fontdata[1];
    font->data = open_shared_memory(font->shmid);
    if (!font->data) {
        free(font);
        return 0;
    }
    return font;
}
void gui_delete_font(font_t *font)
{
    close_shared_memory(font->shmid);
    free(font);
}

int render_char(bitmap_t bitmap, font_t *font, unsigned char c, int x, int y)
{
    int offset = ((int*)font->data)[257 + c];
    //printf("Offset: %d\n", offset);
    int *glyph_data = (int*)(((char*)font->data) + offset);
    int bearing_x = glyph_data[2];
    int bearing_y = glyph_data[3];
    int advance = glyph_data[4];
    int bwidth = glyph_data[5];
    //printf("Width: %d\n", bwidth);
    int bheight = glyph_data[6];
    //printf("Height: %d\n", bheight);
    byte *glyphbitmap = (byte*)&glyph_data[8];
    
    x += bearing_x / 64;
    y -= bearing_y / 64;
    
    //Maﬂe anpassen
    int offsetx = 0;
    int offsety = 0;
    if (x < 0) {
        offsetx = -x;
        bwidth -= offsetx;
    }
    if (y < 0) {
        offsety = -y;
        bheight -= offsety;
    }
    if (x + offsetx + bwidth > bitmap.width) {
        bwidth = bitmap.width - x - offsetx;
    }
    if (y + offsety + bheight > bitmap.height) {
        bheight = bitmap.height - y - offsety;
    }
    switch (bitmap.bpp)
    {
        case 1:
            break;
        case 2:
            break;
        case 4:
            {
            int ix, iy;
            byte color4 = convert_color_24_to_4(0x00000000);
            for (iy = 0; iy < bheight; iy++) {
                for (ix = 0; ix < bwidth; ix++) {
                    if (glyphbitmap[ix + offsetx + (iy + offsety) *  glyph_data[5]] > 128) {
                        draw_dot_4p(bitmap, ix + offsetx + x, iy + offsety + y, color4);
                    }
                }
            }
            }
            break;
        case 8:
            break;
        case 16:
            break;
        case 24:
            break;
        case 32:
            {
            dword *bitmapdata = (dword*)bitmap.data;
            int ix, iy;
            for (iy = 0; iy < bheight; iy++) {
                for (ix = 0; ix < bwidth; ix++) {
                    if (glyphbitmap[ix + offsetx + (iy + offsety) *  glyph_data[5]] > 128) {
                        bitmapdata[ix + offsetx + x + (iy + offsety + y) * bitmap.width] = 0x00000000;
                    }
                }
            }
            }
            break;
    }
    return advance / 64;
}
void render_text(bitmap_t bitmap, font_t *font, const char *string, int x, int y)
{
    int i;
    for (i = 0; i < strlen(string); i++) {
        x += render_char(bitmap, font, string[i], x, y);
    } 
}
dword get_text_width(font_t *font, const char *string)
{
    int i;
    dword length = 0;
    for (i = 0; i < strlen(string); i++) {
        int offset = ((int*)font->data)[257 + string[i]];
        int *glyph_data = (int*)(((char*)font->data) + offset);
        int advance = glyph_data[4];
        length += advance / 64;
    }
    return length;
}

dword get_string_position(font_t *font, const char *string, int x)
{
    if (x < 0) {
        return 0;
    }
    int i;
    dword length = 0;
    for (i = 0; i < strlen(string); i++) {
        int offset = ((int*)font->data)[257 + string[i]];
        int *glyph_data = (int*)(((char*)font->data) + offset);
        int advance = glyph_data[4];
        length += advance / 64;
        if (length > x) {
            return i;
        }
    }
    return strlen(string);
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
    //Ist nicht f¸r uns?
    if (!window) {
        printf("[ LIBGUI ] Fehlerhafte Fenster-ID empfangen!\n");
        rpc_send_dword_response(pid, correlation_id, 0);
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
        if ((window->flags & GUI_WINDOW_FRAME) && (event.data[2] < 11)) {
            if ((event.data[1] > (window->width - 11))
               && (event.data[1] < (window->width - 2))
               && (window->flags & GUI_WINDOW_CLOSE)) {
                event_t newevent;
                newevent.type = EVENT_CLOSE_WINDOW;
                newevent.windowid = event.windowid;
                gui_send_event(newevent);
                rpc_send_dword_response(pid, correlation_id, 1);
            } else {
                gui_start_moving_window(window);
                rpc_send_dword_response(pid, correlation_id, 1);
            }
            return;
        } else {
            rpc_send_dword_response(pid, correlation_id, 1);
            if (!process_event(window, event)) {
                if (gui_callback) gui_callback(*((event_t*)data));
            }
        }
    } else if (event.type == EVENT_KEYBOARD_BUTTON_PRESSED) {
        rpc_send_dword_response(pid, correlation_id, 1);
        if (!process_event(window, event)) {
            if (gui_callback) gui_callback(*((event_t*)data));
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
            
            if (window->title) {
                render_text(window->bitmap, titlefont, window->title, 5, 10);
            }
            
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
        //Sende Antwort erst sp‰ter, damit das Fenster korrekt gerendert wird
        rpc_send_dword_response(pid, correlation_id, 1);
    }
}
