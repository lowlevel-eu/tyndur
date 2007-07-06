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

#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "lostio.h"
#include "syscall.h"

#define _USE_START_
#include "init.h"

#include "video/video.h"
#include "video/bitmap.h"
#include "gui/commands.h"

#include "window.h"

//Interface über RPC
void rpc_gui_interface(pid_t pid,
                      dword correlation_id,
                      size_t data_size,
                      void* data);

size_t control_write(lostio_filehandle_t *file, size_t blocksize,
    size_t blockcount, void *data);
size_t mouse_write(lostio_filehandle_t *file, size_t blocksize,
    size_t blockcount, void *data);

//Treiber etc.
int server_num = 0;
char *videodrivername = "vga";
void *screen_buffer = 0;
bitmap_t screen;
char *inputdrivername = "kbc";
int has_video_driver = 0;
int has_input_driver = 0;


int screen_width = 640;
int screen_height = 480;
int screen_bpp = 4;
int mousex = 160;
int mousey = 100;
int old_mousex = 160;
int old_mousey = 100;
bitmap_t mousepos_clean;

int need_update = 1; //Am Anfang einmal updaten
int mouse_moved = 0;

bitmap_t mouse_bitmap;
sprite_t mouse_cursor;
byte cursor_data[] =
{1,0,0,0,0,0,0,0,
 1,1,0,0,0,0,0,0,
 1,2,1,0,0,0,0,0,
 1,2,2,1,0,0,0,0,
 1,2,2,2,1,0,0,0,
 1,2,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0};

int main(int argc, char **argv)
{
    lostio_init();
    lostio_type_ramfile_use();
    lostio_type_directory_use();
    
    //Argumente auswerten
    //TODO
    //Konfigurationsdateien auswerten
    //TODO 
    
    //Server einrichten
    char name[10];
    if (server_num) {
        //Position angegeben
        snprintf(name, 10, "gui%d", server_num);
        if (init_service_get(name)) {
            printf("[ GUI ] Platz %d schon belegt.\n", server_num);
            return -1;
        }
    } else {
        //Position nicht angegeben
        server_num = 1;
        for(;;) {
            snprintf(name, 10, "gui%d", server_num);
            if (!init_service_get(name)) {
                break;
            }
            server_num++;
        }
    }

    screen.width = 0;
    screen.height = 0;
    screen.bpp = 0;
    screen.data = 0;
    //Videotreiber initialisieren
    if (videodrivername) {
        if (video_init(videodrivername)) {
            //Auflösung heraussuchen
            //TODO
            //Grafik initialisieren
            screen_buffer = set_resolution(screen_width, screen_height, screen_bpp);
            if (screen_buffer) {
                has_video_driver = 1;
                memset(screen_buffer, 0, screen_width * screen_height * screen_bpp / 8);
                video_update_screen();
                screen.width = screen_width;
                screen.height = screen_height;
                screen.bpp = screen_bpp;
                screen.data = screen_buffer;
            } else {
                video_close();
            }
        }
    }
    
    //Font-Server einrichten
    
    //Fenstersystem initialisieren
    init_windows();
    
    //Mauszeiger
    //TODO: bpp etc.
    mouse_bitmap.width = 8;
    mouse_bitmap.height = 8;
    mouse_bitmap.bpp = 32;
    mouse_bitmap.data = malloc(8 * 8 * 4);
    int i;
    dword *bitmap_data = (dword*)mouse_bitmap.data;
    for (i = 0; i < 8 * 8; i++) {
        switch (cursor_data[i])
        {
            case 1:
                bitmap_data[i] = 0xFFFFFF;
                break;
            case 2:
                bitmap_data[i] = 0x000000;
                break;
            default:
                bitmap_data[i] = 0x00FF00;
                break;
        }
    }
    convert_bitmap(&mouse_bitmap, screen_bpp);
    mouse_cursor = convert_bitmap_to_sprite(mouse_bitmap, 0x00FF00);
    /*printf("mouse_cursor: %d Segmente.\n", mouse_cursor.span_count);
    printf("mouse_cursor: %d Pixel.\n", mouse_cursor.pixel_count);
    for (i = 0; i < 10; i++) {
        printf("mouse_cursor: Pixel %d = %08X\n", i, ((dword*)mouse_cursor.pixel_data)[i]);
    }*/
    
    mousepos_clean.width = 16;
    mousepos_clean.height = 16;
    mousepos_clean.bpp = screen_bpp;
    mousepos_clean.data = malloc(16 * 16 * screen_bpp / 8);
    
    //Steuerung des GUI-Servers
    typehandle_t* typehandle_control = malloc(sizeof(typehandle_t));
    typehandle_control->id          = 255;
    typehandle_control->not_found   = NULL;
    typehandle_control->pre_open    = NULL;
    typehandle_control->post_open   = NULL; 
    typehandle_control->read        = NULL; 
    typehandle_control->write       = &control_write;
    typehandle_control->seek        = NULL;
    typehandle_control->close       = NULL;
    lostio_register_typehandle(typehandle_control);
    
    //Maus-Callback
    typehandle_t* typehandle_mouse = malloc(sizeof(typehandle_t));
    typehandle_control->id          = 254;
    typehandle_control->not_found   = NULL;
    typehandle_control->pre_open    = NULL;
    typehandle_control->post_open   = NULL; 
    typehandle_control->read        = NULL; 
    typehandle_control->write       = &mouse_write;
    typehandle_control->seek        = NULL;
    typehandle_control->close       = NULL;
    lostio_register_typehandle(typehandle_mouse);
    
    // vfstree bauen
    vfstree_create_node("/control" , 255, 0, NULL, 0);
    vfstree_create_node("/mouse" , 254, 0, NULL, 0);
    
    //Interface
    register_message_handler("GUIFNCTS", rpc_gui_interface);

    // Registrieren
    init_service_register(name);
    
    //Input-Callbacks einrichten
    if (inputdrivername) {
        qword timeout = get_tick_count() + 5000000;
        while (init_service_get(inputdrivername) == 0)
        {
            yield();
            if(get_tick_count() > timeout)
            {
                printf("[ GUI ] Inputtreiber wurde nicht gefunden.\n");

                inputdrivername = 0;
                break;
            }
        }
    }
    if (inputdrivername) {
        char tmp[30];
        snprintf(tmp, 30, "%s:/mouse/callback", inputdrivername);
        FILE *mousecb = fopen(tmp, "w");
        snprintf(tmp, 30, "add %s:/mouse", name);
        fwrite(tmp ,strlen(tmp) + 1,1,mousecb);
        fclose(mousecb);
    }
    
    while(TRUE) {
        yield();
        if (need_update) {
            int curr_mousex = mousex;
            int curr_mousey = mousey;
            need_update = 0;
            if (screen.data) {
                memset(screen.data, 0, screen_width * screen_height * screen_bpp / 8);
                //Fenster rendern
                bitmap_draw_bitmap(screen, mousepos_clean, old_mousex & ~0x7, old_mousey & ~0x7, 16, 16);
                draw_all_windows();
                //Mauszeiger rendern
                
                bitmap_draw_bitmap(mousepos_clean, screen, -(curr_mousex & ~0x7), -(curr_mousey & ~0x7), screen_width, screen_height);
                bitmap_draw_sprite(screen, mouse_cursor, curr_mousex, curr_mousey);
                
                video_update_screen();
                 
                old_mousex = curr_mousex;
                old_mousey = curr_mousey;
            }
        } else if (mouse_moved) {
            int curr_mousex = mousex;
            int curr_mousey = mousey;
            mouse_moved = 0;
            if (screen.data) {
                //Mauszeiger rendern
                bitmap_draw_bitmap(screen, mousepos_clean, old_mousex & ~0x7, old_mousey & ~0x7, 16, 16);
                bitmap_draw_bitmap(mousepos_clean, screen, -(curr_mousex & ~0x7), -(curr_mousey & ~0x7), screen_width, screen_height);
                bitmap_draw_sprite(screen, mouse_cursor, curr_mousex, curr_mousey);
                int redrawtop = (curr_mousey < (old_mousey & ~0x7))?curr_mousey:(old_mousey & ~0x7);
                int redrawbottom = ((curr_mousey + 6) > ((old_mousey & ~0x7) + 16))?(curr_mousey + 6):((old_mousey & ~0x7) + 16);
                int redrawleft = (curr_mousex < (old_mousex & ~0x7))?curr_mousex:(old_mousex & ~0x7);
                int redrawright = ((curr_mousex + 6) > ((old_mousex & ~0x7) + 16))?(curr_mousex + 6):((old_mousex & ~0x7) + 16);
                video_update_screen_partially(redrawleft, redrawtop, redrawright - redrawleft, redrawbottom - redrawtop);
                //printf("Neugezeichnet: %d * %d!\n", redrawright - redrawleft, redrawbottom - redrawtop);
                
                old_mousex = curr_mousex;
                old_mousey = curr_mousey;
            }
        }
    }
}

void rpc_gui_interface(pid_t pid,
                       dword correlation_id,
                       size_t data_size,
                       void* data)
{
    p();
    //Prozess ist angemeldet?
    
    //printf("Befehl.\n");
    //position_t position;
    //dimension_t size;
    
    int *command = data;
    if (data_size < 4) {
        rpc_send_dword_response(pid, correlation_id, 0);
        return;
    }
    //printf("[ GUI ] Befehl: %d\n", command[0]);
    switch (command[0]) {
        case GUI_CREATE_WINDOW:
            if (data_size < 24) {
                rpc_send_dword_response(pid, correlation_id, 0);
                return;
            }
            window_t *newwindow = create_window(pid, command[1], command[2],
                command[3], command[4], command[5]);
            rpc_send_response(pid, correlation_id, sizeof(window_t),
                (char*)newwindow);
            break;
        case GUI_CLOSE_WINDOW:
            if (data_size < 8) {
                rpc_send_dword_response(pid, correlation_id, 0);
                return;
            }
            delete_window(pid, command[1]);
            rpc_send_dword_response(pid, correlation_id, 1);
            break;
        case GUI_REPAINT_WINDOW:
            if (data_size < 8) {
                rpc_send_dword_response(pid, correlation_id, 0);
                return;
            }
            rpc_send_dword_response(pid, correlation_id, redraw_window(command[1]));
            break;
        case GUI_MOVE_WINDOW:
            if (data_size < 8) {
                rpc_send_dword_response(pid, correlation_id, 0);
                return;
            }
            rpc_send_dword_response(pid, correlation_id, start_moving_window(command[1]));
            break;
        case GUI_RESIZE_WINDOW:
            if (data_size < 12) {
                rpc_send_dword_response(pid, correlation_id, 0);
                return;
            }
            rpc_send_dword_response(pid, correlation_id, start_resizing_window(command[1], command[2]));
            break;
        case GUI_SET_CALLBACK_FUNCTION:
            if (data_size < 16) {
                rpc_send_dword_response(pid, correlation_id, 0);
                return;
            }
            set_event_receiver(pid, (char*)&command[1]);
            rpc_send_dword_response(pid, correlation_id, 1);
            break;
    }
    v();
}

size_t control_write(lostio_filehandle_t *file,
                     size_t blocksize,
                     size_t blockcount,
                     void *data)
{
    //TODO
    return 0;
}

size_t mouse_write(lostio_filehandle_t *file, size_t blocksize,
                   size_t blockcount, void *data)
{
    //printf("Mouse-Event.\n");
    if (blocksize * blockcount < 12) return blocksize * blockcount;
    int old_mousex = mousex;
    int old_mousey = mousey;
    mousex += ((int*)data)[0];
    mousey += ((int*)data)[1];
    //An Bildschirmrand angekommen?
    if (mousex < 0) {
        ((int*)data)[0] = -old_mousex;
        mousex = 0;
    }
    if (mousex > screen_width - 1) {
        ((int*)data)[0] = screen_width - 1 - old_mousex;
        mousex = screen_width - 1;
    }
    if (mousey < 0) {
        ((int*)data)[1] = -old_mousey;
        mousey = 0;
    }
    if (mousey > screen_height - 1) {
        ((int*)data)[1] = screen_height - 1 - old_mousey;
        mousey = screen_height - 1;
    }
    //Events auslösen
    if ((((int*)data)[0] != 0) || (((int*)data)[1] != 0)) {
        inject_mouse_move(((int*)data)[0], ((int*)data)[1]);
    }
    if (((char*)data)[9]) {
        int i;
        for (i = 0; i < 8; i++) {
            if ((((char*)data)[9] >> i) & 1) {
                inject_mouse_button_pressed(i + 1);
            }
        }
    }
    if (((char*)data)[10]) {
        int i;
        for (i = 0; i < 8; i++) {
            if ((((char*)data)[10] >> i) & 1) {
                inject_mouse_button_released(i + 1);
            }
        }
    }
    //Bildschirmbereich neu zeichnen
    mouse_moved = 1;
    return blocksize * blockcount;
}
