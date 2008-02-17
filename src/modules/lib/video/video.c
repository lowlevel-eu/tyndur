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

#include "video/video.h"
#include "video/commands.h"

#include "types.h"
#include "stdio.h"
#include "stdlib.h"
#include "rpc.h"
#include <syscall.h>
#include "init.h"

FILE *video_driver_control = 0;
int video_command_data[10];

dword video_buffer_id = 0;
void *video_buffer = 0;

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
    if (video_buffer_id) {
        close_shared_memory(video_buffer_id);
        video_buffer_id = 0;
        video_buffer = 0;
    }
    //Registrierung auflösen
    video_command_data[0] = VIDEO_UNREGISTER;
    rpc_get_dword(video_driver_control->pid,
                  "VIDEODRV",
                  sizeof(int),
                  (char*)video_command_data);
    //Schließe Verbindung zum Treiber
    fclose(video_driver_control);
    video_driver_control = 0;
}

void *set_resolution(dword width, dword height, dword bpp)
{
    if (!video_driver_control) {
        return 0;
    }
    if (video_buffer_id) {
        close_shared_memory(video_buffer_id);
        video_buffer_id = 0;
        video_buffer = 0;
    }
    video_command_data[0] = VIDEO_SET_RESOLUTION;
    video_command_data[1] = width;
    video_command_data[2] = height;
    video_command_data[3] = bpp;
    int result = rpc_get_dword(video_driver_control->pid,
                  "VIDEODRV",
                  sizeof(int) * 4,
                  (char*)video_command_data);
    if (!result) return 0;
    video_buffer_id = result;
    video_buffer = open_shared_memory(video_buffer_id);
    return video_buffer;
}
resolution_t video_get_resolution(void)
{
    resolution_t res;
    res.width = 0;
    res.height = 0;
    res.bpp = 0;
    if (!video_driver_control) {
        return res;
    }
    video_command_data[0] = VIDEO_GET_RESOLUTION;
    response_t *response = rpc_get_response(video_driver_control->pid, "VIDEODRV", sizeof(int), (char*)video_command_data);
    if (response->data_length != sizeof(resolution_t)) {
        return res;
    }
    memcpy(&res, response->data, sizeof(resolution_t));
    free(response->data);
    free(response);
    return res;
}
resolution_list_t video_get_resolutions(void)
{
    resolution_list_t list;
    list.res_count = 0;
    list.resolutions = 0;
    if (!video_driver_control) {
        return list;
    }
    video_command_data[0] = VIDEO_GET_RESOLUTION;
    response_t *response = rpc_get_response(video_driver_control->pid, "VIDEODRV", sizeof(int), (char*)video_command_data);
    if (response->data_length <= 4) {
        return list;
    }
    list.res_count = (response->data_length - 4) / sizeof(resolution_t);
    list.resolutions = malloc(list.res_count * sizeof(resolution_t));
    memcpy(list.resolutions, response->data + 4, list.res_count * sizeof(resolution_t));
    free(response->data);
    free(response);
    return list;
}
palette_t video_get_palette(void)
{
    palette_t palette;
    palette.color_count = 0;
    palette.colors = 0;
    if (!video_driver_control) {
        return palette;
    }
    video_command_data[0] = VIDEO_GET_RESOLUTION;
    response_t *response = rpc_get_response(video_driver_control->pid, "VIDEODRV", sizeof(int), (char*)video_command_data);
    if (response->data_length <= 4) {
        return palette;
    }
    palette.color_count = (response->data_length - 4) / 4;
    palette.colors = malloc(palette.color_count * 4);
    memcpy(palette.colors, response->data + 4, palette.color_count * 4);
    free(response->data);
    free(response);
    return palette;
}

void *get_screen_buffer(void)
{
    return video_buffer;
}

int video_update_screen(void)
{
    if (!video_driver_control) {
        return 0;
    }
    video_command_data[0] = VIDEO_NEED_UPDATE;
    return rpc_get_dword(video_driver_control->pid,
                         "VIDEODRV",
                         sizeof(int),
                         (char*)video_command_data);
}
int video_update_screen_partially(int x, int y, int width, int height)
{
    if (!video_driver_control) {
        return 0;
    }
    video_command_data[0] = VIDEO_NEED_PARTIAL_UPDATE;
    video_command_data[1] = x;
    video_command_data[2] = y;
    video_command_data[3] = width;
    video_command_data[4] = height;
    return rpc_get_dword(video_driver_control->pid,
                         "VIDEODRV",
                         sizeof(int) * 5,
                         (char*)video_command_data);
}
