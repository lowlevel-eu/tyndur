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

#include "video/video.h"
#include "video/commands.h"

#include "types.h"
#include "stdio.h"
#include "stdlib.h"
#include "rpc.h"
#include "init.h"

FILE *video_driver_control = 0;
int video_command_data[10];

int *video_command_chain = 0;
int video_command_chain_memsize = 0;
int video_command_chain_size = 0;

int video_init(char *driver_name)
{
    if (!driver_name) {
        return 0;
    }
    //Suche Treiber
    qword timeout = get_tick_count() + 5000000;
    while (init_service_get(driver_name) == 0) {
        yield();
        if(get_tick_count() > timeout) {
            puts("[ LIBVIDEO ] Konnte Videotreiber nicht finden!");
            return 0;
        }
    }
    //Öffne Videotreiber
    char tmp[30];
    sprintf(tmp, "%s:/information", driver_name);
    video_driver_control = fopen(tmp, "r");
    if (!video_driver_control) {
        printf("[ LIBVIDEO ] Konnte Videotreiber nicht oeffnen.\n");
        return 0;
    } else {
        video_command_data[0] = VIDEO_REGISTER;
        if (!rpc_get_dword(video_driver_control->pid,
                           "VIDEODRV",
                           sizeof(int),
                           (char*)video_command_data)) {
            fclose(video_driver_control);
            video_driver_control = 0;
            printf("[ LIBVIDEO ] Konnte Prozess nicht registrieren.\n");
            return 0;
        }
        video_command_data[0] = VIDEO_INIT;
        if (!rpc_get_dword(video_driver_control->pid,
             "VIDEODRV",
             sizeof(int) * 4,
             (char*)video_command_data)) {
            video_command_data[0] = VIDEO_UNREGISTER;
            rpc_get_dword(video_driver_control->pid,
                          "VIDEODRV",
                          sizeof(int),
                          (char*)video_command_data);
            fclose(video_driver_control);
            video_driver_control = 0;
            printf("[ LIBVIDEO ] Konnte Videotreiber nicht initialisieren.\n");
            return 0;
        }

    }
    return 1;
}
void video_close(void)
{
    if (!video_driver_control) {
        return;
    }
    //TODO: Registrierung auflösen
    //Schließe Verbindung zum Treiber
    fclose(video_driver_control);
    video_driver_control = 0;
}

int video_start_command_chain(void)
{
    if (!video_driver_control) {
        return 0;
    }
    if (video_command_chain) {
        return 1;
    }
    video_command_chain = malloc(1000);
    if (!video_command_chain) {
        return 0;
    }
    video_command_chain[0] = VIDEO_COMMAND_CHAIN;
    video_command_chain_size = 1;
    video_command_chain_memsize = 1000;
    return 1;
}
int video_send_command_chain(void)
{
    if (!video_driver_control) {
        return 0;
    }
    if (!video_command_chain) {
        return 0;
    }
    int result = rpc_get_dword(video_driver_control->pid, "VIDEODRV",
        sizeof(int) * video_command_chain_size, (char*)video_command_chain);
    free(video_command_chain);
    video_command_chain = 0;
    video_command_chain_memsize = 0;
    video_command_chain_size = 0;
    return result;
}

int video_enlarge_command_chain(void)
{
    if (!video_command_chain) {
        return 0;
    }
    video_command_chain = realloc(video_command_chain,
        video_command_chain_memsize + 1000);
    if (!video_command_chain) {
        video_command_chain = 0;
        video_command_chain_memsize = 0;
        video_command_chain_size = 0;
        return 0;
    }
    video_command_chain_memsize += 1000;
    return 1;
}

int video_draw_dot(int x, int y, int color)
{
    if (!video_driver_control) {
        return 0;
    }
    if (video_command_chain) {
        if (video_command_chain_size + 4 > video_command_chain_memsize) {
            video_enlarge_command_chain();
        }
        video_command_chain[video_command_chain_size] = VIDEO_DRAW_DOT;
        video_command_chain[video_command_chain_size + 1] = x;
        video_command_chain[video_command_chain_size + 2] = y;
        video_command_chain[video_command_chain_size + 3] = color;
        video_command_chain_size += 4;
        return 1;
    } else {
        video_command_data[0] = VIDEO_DRAW_DOT;
        video_command_data[1] = x;
        video_command_data[2] = y;
        video_command_data[3] = color;
        return rpc_get_dword(video_driver_control->pid, "VIDEODRV",
            sizeof(int) * 4, (char*)video_command_data);
    }
}
int video_draw_rect(int x, int y, int width, int height, int color)
{
    if (!video_driver_control) {
        return 0;
    }
    if (video_command_chain) {
        if (video_command_chain_size + 6 > video_command_chain_memsize) {
            video_enlarge_command_chain();
        }
        video_command_chain[video_command_chain_size] = VIDEO_DRAW_RECT;
        video_command_chain[video_command_chain_size + 1] = x;
        video_command_chain[video_command_chain_size + 2] = y;
        video_command_chain[video_command_chain_size + 3] = width;
        video_command_chain[video_command_chain_size + 4] = height;
        video_command_chain[video_command_chain_size + 5] = color;
        video_command_chain_size += 6;
        return 1;
    } else {
        video_command_data[0] = VIDEO_DRAW_RECT;
        video_command_data[1] = x;
        video_command_data[2] = y;
        video_command_data[3] = width;
        video_command_data[4] = height;
        video_command_data[5] = color;
        return rpc_get_dword(video_driver_control->pid,
                             "VIDEODRV",
                             sizeof(int) * 6,
                             (char*)video_command_data);
    }
}
int video_clear_screen(int color)
{
    if (!video_driver_control) {
        return 0;
    }
    if (video_command_chain) {
        if (video_command_chain_size + 2 > video_command_chain_memsize) {
            video_enlarge_command_chain();
        }
        video_command_chain[video_command_chain_size] = VIDEO_CLEAR_SCREEN;
        video_command_chain[video_command_chain_size + 1] = color;
        video_command_chain_size += 2;
        return 1;
    } else {
        video_command_data[0] = VIDEO_CLEAR_SCREEN;
        video_command_data[1] = color;
        return rpc_get_dword(video_driver_control->pid,
                             "VIDEODRV",
                             sizeof(int) * 2,
                             (char*)video_command_data);
    }
}

int video_draw_bitmap(int bitmap, int x, int y, int transparent)
{
    if (!video_driver_control) {
        return 0;
    }
    if (!bitmap) {
        return 0;
    }
    if (video_command_chain) {
        if (video_command_chain_size + 5 > video_command_chain_memsize) {
            video_enlarge_command_chain();
        }
        video_command_chain[video_command_chain_size] = VIDEO_DRAW_BITMAP;
        video_command_chain[video_command_chain_size + 1] = bitmap;
        video_command_chain[video_command_chain_size + 2] = x;
        video_command_chain[video_command_chain_size + 3] = y;
        video_command_chain[video_command_chain_size + 4] = transparent;
        video_command_chain_size += 5;
        return 1;
    } else {
        video_command_data[0] = VIDEO_DRAW_BITMAP;
        video_command_data[1] = bitmap;
        video_command_data[2] = x;
        video_command_data[3] = y;
        video_command_data[4] = transparent;
        return rpc_get_dword(video_driver_control->pid,
                             "VIDEODRV",
                             sizeof(int) * 5,
                             (char*)video_command_data);
    }
}

int video_load_bitmap(int width, int height, void *data)
{
    int tmpdata[20001];
    int datasent = 0;
    if (!video_driver_control) {
        return 0;
    }
    if (!data) {
        return 0;
    }
    if (!width || !height) {
        return 0;
    }
    
    //Kopiere Befehl und Bitmap in temporären Speicher
    video_command_data[0] = VIDEO_CREATE_BITMAP;
    video_command_data[1] = width;
    video_command_data[2] = height;
    dword result = rpc_get_dword(video_driver_control->pid,
                                 "VIDEODRV",
                                 sizeof(int) * 3,
                                 (char*)video_command_data);
    if (!result) {
        return 0;
    }
    while (datasent < (width * height)) {
        tmpdata[0] = VIDEO_MORE_DATA;
        int size = 20000;
        if ((width * height - datasent) < 20000) {
            size = width * height - datasent;
        }
        memcpy(&tmpdata[1], &((int*)data)[datasent], size * sizeof(int));
        result = rpc_get_dword(video_driver_control->pid,
                               "VIDEODRV",
                               sizeof(int) * (size + 1),
                               (char*)tmpdata);
        datasent += size;
    }
    video_command_data[0] = 11;
    result = rpc_get_dword(video_driver_control->pid,
                           "VIDEODRV",
                           sizeof(int),
                           (char*)video_command_data);
    return result;
}
int video_delete_bitmap(int bitmap)
{
    if (!video_driver_control) {
        return 0;
    }
    video_command_data[0] = VIDEO_DELETE_BITMAP;
    video_command_data[1] = bitmap;
    return rpc_get_dword(video_driver_control->pid,
                         "VIDEODRV",
                         sizeof(int) * 2,
                         (char*)video_command_data);
}

int video_update_screen(void)
{
    if (!video_driver_control) {
        return 0;
    }
    if (video_command_chain) {
        if (video_command_chain_size + 1 > video_command_chain_memsize) {
            video_enlarge_command_chain();
        }
        video_command_chain[video_command_chain_size] = VIDEO_UPDATE_SCREEN;
        video_command_chain_size += 1;
        return 1;
    } else {
        video_command_data[0] = VIDEO_UPDATE_SCREEN;
        return rpc_get_dword(video_driver_control->pid,
                             "VIDEODRV",
                             sizeof(int),
                             (char*)video_command_data);
    }
}
int video_update_partly(int x, int y, int width, int height)
{
    if (!video_driver_control) {
        return 0;
    }
    if (video_command_chain) {
        if (video_command_chain_size + 5 > video_command_chain_memsize) {
            video_enlarge_command_chain();
        }
        video_command_chain[video_command_chain_size] = VIDEO_UPDATE_PARTLY;
        video_command_chain[video_command_chain_size + 1] = x;
        video_command_chain[video_command_chain_size + 2] = y;
        video_command_chain[video_command_chain_size + 3] = width;
        video_command_chain[video_command_chain_size + 4] = height;
        video_command_chain_size += 5;
        return 1;
    } else {
        video_command_data[0] = VIDEO_UPDATE_PARTLY;
        video_command_data[1] = x;
        video_command_data[2] = y;
        video_command_data[3] = width;
        video_command_data[4] = height;
        return rpc_get_dword(video_driver_control->pid,
                             "VIDEODRV",
                             sizeof(int) * 5,
                             (char*)video_command_data);
    }
}
