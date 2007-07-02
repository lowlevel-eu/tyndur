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
 
#include "time.h"

#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "lostio.h"
#include "syscall.h"

#define _USE_START_
#include "init.h"

#include "video/video.h"

#include "bitmap.h"
#include "pcf.h"
#include "gui.h"

#include "widget.h"
#include "button.h"
#include "window.h"
#include "panel.h"
#include "textbox.h"

#if defined(__i386__)

static __inline__ unsigned long long rdtsc(void)
{
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}
#elif defined(__x86_64__)

typedef unsigned long long int unsigned long long;

static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#elif defined(__powerpc__)

typedef unsigned long long int unsigned long long;

static __inline__ unsigned long long rdtsc(void)
{
    unsigned long long int result=0;
    unsigned long int upper, lower,tmp;
    __asm__ volatile(
            "0:                  \n"
            "\tmftbu   %0           \n"
            "\tmftb    %1           \n"
            "\tmftbu   %2           \n"
            "\tcmpw    %2,%0        \n"
            "\tbne     0b         \n"
    : "=r"(upper),"=r"(lower),"=r"(tmp)
                    );
    result = upper;
    result = result<<32;
    result = result|lower;

    return(result);
}

#endif

//Interface über RPC
void rpc_gui_interface(pid_t pid,
                      dword correlation_id,
                      size_t data_size,
                      void* data);

size_t control_write(lostio_filehandle_t *file, size_t blocksize,
    size_t blockcount, void *data);
size_t mouse_write(lostio_filehandle_t *file, size_t blocksize,
    size_t blockcount, void *data);

int server_num = 0;
char *videodrivername = "vga";
char *inputdrivername = "kbc";
int has_video_driver = 0;

int testbitmap[256][256];

int screen_width = 640;
int screen_height = 480;
int mousex = 160;
int mousey = 100;

int main(int argc, char **argv)
{
    lostio_init();
    lostio_type_ramfile_use();
    lostio_type_directory_use();
    
    //Argumente auswerten
    
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
    
    
    while (init_service_get("floppy") == 0)
    {
        yield();
    }
    font_t times = load_pcf_font("floppy:/devices/fd0|fat:/data/times.pcf");
    font_t helvetica = load_pcf_font("floppy:/devices/fd0|fat:/data/helvetic.pcf");
    set_default_font(helvetica);
    register_widget_class(-1, button_class);
    register_widget_class(-1, window_class);
    register_widget_class(-1, panel_class);
    register_widget_class(-1, textbox_class);
    init_root_widget();
    
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
    
    if (videodrivername) {
        if (video_init(videodrivername)) {
            has_video_driver = 1;
            
            //Ein paar Tests
            /*
            video_clear_screen(0xFF7777FF);
            video_draw_dot(10, 10, 0xFFFF0000);
            video_draw_rect(0, 0, 50, 50, 0xFF00FF00);
            video_update_screen();
            
            //Test-Bitmap erstellen
            int ix, iy;
            for (ix = 0; ix < 256; ix++) {
                for (iy = 0; iy < 256; iy++) {
                    testbitmap[iy][ix] = (iy << 24)
                                       + (ix << 16)
                                       + (ix << 8)
                                       + ix;
                }
            }
            printf("[ GUI ] Erstelle Bitmap.\n");
            int bitmap = video_load_bitmap(256, 256, testbitmap);
            if (!bitmap) printf("[ GUI ] Konnte Bitmap nicht erstellen.\n");*/
            /*position_t position; position.x = 10, position.y = 10;
            dimension_t size; size.width = 100, size.height = 30;
            widget_t button = create_widget_ex("button", 0, position, size, -1);
            set_label(button, "Testbutton 1");
            position.x = 100, position.y = 100;
            size.width = 300, size.height = 200;
            widget_t window = create_widget_ex("window", 0, position, size, -1);
            set_label(window, "Testfenster");
            position.x = 10, position.y = 20;
            size.width = 100, size.height = 30;
            widget_t button2 = create_widget_ex("button", window, position, size,
                -1);
            set_label(button2, "Testbutton 2");*/
            
            /*bitmap_t testbitmap = load_bitmap_bmp("floppy:/devices/fd0|fat:/data/test.bmp");
            int bitmap = 0;
            if (testbitmap.data) bitmap = video_load_bitmap(testbitmap.width, testbitmap.height, testbitmap.data);*/
            

            /*int textbitmap = 0;
            bitmap_t testtext = render_string(times,
                "Dies ist ein Testsatz, der mithilfe eines PCF-Fonts gerendert wird.");
            if (testtext.data) textbitmap = video_load_bitmap(testtext.width,
                testtext.height, testtext.data);*/
            
            long long lasttime1 = 0;
            long long lasttime2 = 0;
            long long lasttime3 = 0;
            long long lasttime4 = 0;
            long long lasttime = 1;
            int red = 0;
            for (;;) {
                yield();
                //Führe Befehle aus
                p();
                video_start_command_chain();
                long long time1 = rdtsc();
                video_clear_screen(0xFF7777FF);
                long long time2 = rdtsc();
                
                draw_all();
                
                //Anzeige der benötigten Zeit
                long long time3 = rdtsc();
                /*video_draw_rect(0, 0, 20, 20, 0xFF000000 + red * 0xFF0000);
                //printf("Zeit: %d\n", (int)lasttime);
                int width1 = (int)((float)lasttime1 * 200 / (float)lasttime);
                int width2 = (int)((float)lasttime2 * 200 / (float)lasttime);
                int width3 = (int)((float)lasttime3 * 200 / (float)lasttime);
                int width4 = (int)((float)lasttime4 * 200 / (float)lasttime);
                video_draw_rect(0, 40, width1, 5, 0xFF0000FF);
                video_draw_rect(0, 50, width2, 5, 0xFF0000FF);
                video_draw_rect(0, 60, width3, 5, 0xFF0000FF);
                video_draw_rect(0, 70, width4, 5, 0xFF0000FF);
                video_draw_rect(width1, 40, 200 - width1, 5, 0xFF00FF00);
                video_draw_rect(width2, 50, 200 - width2, 5, 0xFF00FF00);
                video_draw_rect(width3, 60, 200 - width3, 5, 0xFF00FF00);
                video_draw_rect(width4, 70, 200 - width4, 5, 0xFF00FF00);*/
                //if (bitmap) video_draw_bitmap(bitmap, 50, 50, 0);
                //if (textbitmap) video_draw_bitmap(textbitmap, 100, 450, 0);
                video_draw_rect(mousex - 1, mousey - 1, 3, 3, 0xFFFF0000);
                long long time4 = rdtsc();
                video_update_screen();
                v();
                //printf("Rendering.\n");
                video_send_command_chain();
                //unlock_mutex(&mutex);
                long long time5 = rdtsc();
                lasttime1 = time2 - time1;
                lasttime2 = time3 - time2;
                lasttime3 = time4 - time3;
                lasttime4 = time5 - time4;
                lasttime = time5 - time1;
                //video_update_partly(0, 0, 50, 50);
                //video_update_partly(screen_width - 50, screen_height - 50, 50, 50);
                red++;
                if (red == 2) red = 0;
            }
        } else {
            has_video_driver = 0;
        }
    }
    
    while(TRUE) {
        wait_for_rpc();
    }
}

void rpc_gui_interface(pid_t pid,
                       dword correlation_id,
                       size_t data_size,
                       void* data)
{
    //Prozess ist angemeldet?
    
    //printf("Befehl.\n");
    position_t position;
    dimension_t size;
    
    int *command = data;
    if (data_size < 4) {
        rpc_send_dword_response(pid, correlation_id, 0);
        return;
    }
    //printf("[ GUI ] Befehl: %d\n", command[0]);
    switch (command[0]) {
        case GUI_CREATE_WIDGET:
            if (data_size < 24) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            int id = create_widget_id();
            int result = (int)create_widget(id, strdup((char*)&command[1]),
                command[5], pid);
            if (result) {
                rpc_send_dword_response(pid, correlation_id, id);
            } else {
                rpc_send_dword_response(pid, correlation_id, 0);
            }
            break;
        case GUI_CREATE_WIDGET_EX:
            if (data_size < 40) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            position.x = command[6];
            position.y = command[7];
            size.width = command[8];
            size.height = command[9];
            id = create_widget_id();
            //printf("id: %d\n", id);
            result = (int)create_widget_ex(id, strdup((char*)&command[1]),
                command[5], position, size, pid);
            if (result) {
                rpc_send_dword_response(pid, correlation_id, id);
            } else {
                rpc_send_dword_response(pid, correlation_id, 0);
            }
            break;
        case GUI_DELETE_WIDGET:
            if (data_size < 8) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            delete_widget(command[1]);
             rpc_send_dword_response(pid, correlation_id, 1);
            break;

        case GUI_SET_POSITION:
            if (data_size < 16) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            position_t position;
            position.x = command[2];
            position.y = command[3];
            set_position(command[1], position);
            rpc_send_dword_response(pid, correlation_id, 1);
            break;
        case GUI_SET_SIZE:
            if (data_size < 16) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            dimension_t size;
            size.width = command[2];
            size.height = command[3];
            set_size(command[1], size);
            rpc_send_dword_response(pid, correlation_id, 1);
            break;
        case GUI_SET_LABEL:
            if (data_size < 12) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            set_label(command[1], (char*)&command[2]);
            rpc_send_dword_response(pid, correlation_id, 1);
            break;

        case GUI_SET_CALLBACK:
            if (data_size < 16) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            rpc_send_dword_response(pid, correlation_id,
                register_callback_function(pid, (char*)&command[1]));
            break;
    }
    //printf("Befehl Ende.\n");
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
    int oldmousex = mousex;
    int oldmousey = mousey;
    mousex += ((int*)data)[0];
    mousey += ((int*)data)[1];
    //An Bildschirmrand angekommen?
    if (mousex < 0) {
        ((int*)data)[0] = -oldmousex;
        mousex = 0;
    }
    if (mousex > screen_width - 1) {
        ((int*)data)[0] = screen_width - 1 - oldmousex;
        mousex = screen_width - 1;
    }
    if (mousey < 0) {
        ((int*)data)[1] = -oldmousey;
        mousey = 0;
    }
    if (mousey > screen_height - 1) {
        ((int*)data)[1] = screen_height - 1 - oldmousey;
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
    return blocksize * blockcount;
}
