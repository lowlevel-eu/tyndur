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

#include "gui/gui.h"

#include "types.h"
#include "stdio.h"
#include "rpc.h"
#include "init.h"
#include "stdlib.h"

FILE *gui_control = 0;
int gui_command_data[10];

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
        
        gui_command_data[0] = 3;
        memcpy(&gui_command_data[1], "LIBGUICB", 9);
        if (!rpc_get_dword(gui_control->pid, "GUIFNCTS", sizeof(int) * 4,
            (char*)gui_command_data)) {
            fclose(gui_control);
            gui_control = 0;
            printf("[ LIBGUI ] Konnte Callback nicht initialisieren.\n");
            return 0;
        }
    }

    return 1;
}
int close_gui(void)
{
    if (!gui_control) {
        return 0;
    }
    fclose(gui_control);
    return 1;
}

widget_t gui_create_widget(char *class, widget_t parent)
{
    printf("[ LIBGUI ] Erstelle Widget (%s).\n", class);
    if (!gui_control) {
        return 0;
    }
    gui_command_data[0] = 0;
    strcpy((char*)&(gui_command_data[1]), class);
    gui_command_data[5] = parent;
    return rpc_get_dword(gui_control->pid,
                         "GUIFNCTS",
                         sizeof(int) * 6,
                         (char*)gui_command_data);
}
widget_t gui_create_widget_ex(char *class, widget_t parent, position_t position, dimension_t size)
{
    printf("[ LIBGUI ] Erstelle Widget (ex) (%s).\n", class);
    if (!gui_control) {
        printf("[ LIBGUI ] GUI nicht initialisiert!\n");
        return 0;
    }
    gui_command_data[0] = 1;
    strcpy((char*)&(gui_command_data[1]), class);
    gui_command_data[5] = parent;
    gui_command_data[6] = position.x;
    gui_command_data[7] = position.y;
    gui_command_data[8] = size.width;
    gui_command_data[9] = size.height;
    return rpc_get_dword(gui_control->pid,
                         "GUIFNCTS",
                         sizeof(int) * 10,
                         (char*)gui_command_data);
}
int gui_delete_widget(widget_t widget)
{
    return 0;
}

int gui_set_position(widget_t widget, position_t position)
{
    if (!gui_control) {
        return 0;
    }
    gui_command_data[0] = 4;
    gui_command_data[1] = widget;
    gui_command_data[2] = position.x;
    gui_command_data[3] = position.y;
    return rpc_get_dword(gui_control->pid,
                         "GUIFNCTS",
                         sizeof(int) * 4,
                         (char*)gui_command_data);
}
int gui_set_size(widget_t widget, dimension_t size)
{
    if (!gui_control) {
        return 0;
    }
    gui_command_data[0] = 5;
    gui_command_data[1] = widget;
    gui_command_data[2] = size.width;
    gui_command_data[3] = size.height;
    return rpc_get_dword(gui_control->pid,
                         "GUIFNCTS",
                         sizeof(int) * 4,
                         (char*)gui_command_data);
}
int gui_set_label(widget_t widget, char *label)
{
    if (!gui_control) {
        return 0;
    }
    int *command = malloc((3 + strlen(label) / sizeof(int)) * sizeof(int));
    command[0] = 6;
    command[1] = widget;
    strcpy((char*)&command[2], label);
    int result = rpc_get_dword(gui_control->pid,
                         "GUIFNCTS",
                         sizeof(int) * (3 + strlen(label) / sizeof(int)),
                         (char*)command);
    free(command);
    return result;
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
    printf("[ LIBGUI ] Callback!\n");
    rpc_send_dword_response(pid, correlation_id, 1);
    if (gui_callback) gui_callback(*((event_t*)data));
}
