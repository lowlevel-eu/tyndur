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

#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "lostio.h"
#include "ports.h"
#include "syscall.h"

#include "driver.h"
#include "video/commands.h"

#define _USE_START_
#include "init.h"

read_hdl_reply information_read(lostio_filehandle_t *file, size_t blocksize,
    size_t blockcount);

//Interface über RPC
void rpc_driver_interface(pid_t pid,
                          dword correlation_id,
                          size_t data_size,
                          void* data);

int owner = -1;

int main(int argc, char **argv)
{
    lostio_init();
    lostio_type_ramfile_use();
    lostio_type_directory_use();
    
    reserve_ressources();
    
    //Informationen über den Treiber
    typehandle_t* typehandle_information = malloc(sizeof(typehandle_t));
    typehandle_information->id          = 255;
    typehandle_information->not_found   = NULL;
    typehandle_information->pre_open    = NULL;
    typehandle_information->post_open   = NULL; 
    typehandle_information->read        = &information_read; 
    typehandle_information->write       = NULL;
    typehandle_information->seek        = NULL;
    typehandle_information->close       = NULL;
    lostio_register_typehandle(typehandle_information);
    
    vfstree_create_node("/information" , 255, 0, NULL, 0);
    
    register_message_handler("VIDEODRV", rpc_driver_interface);
    
    init_service_register(DRIVER_NAME);

    main_loop();
    return 0;
}

read_hdl_reply information_read(lostio_filehandle_t *file, size_t blocksize,
    size_t blockcount)
{
    read_hdl_reply ret;

    size_t totalsize = blocksize * blockcount;
    if (totalsize < strlen(DRIVER_INFO) + 1) {
        ret.data = 0;
        ret.size = 0;

        return ret;
    }
    ret.data = DRIVER_INFO;
    ret.size = strlen(DRIVER_INFO) + 1;
    return ret;
}

void rpc_driver_interface(pid_t pid,
                          dword correlation_id,
                          size_t data_size,
                          void* data)
{
    //printf("VGA-RPC!\n");
    p();
    int result;
    //Befehlsinformation auslesen und Befehl ausführen
    int *command = data;
    if (data_size < sizeof(int)) {
        rpc_send_dword_response(pid, correlation_id, 0);
        return;
    }
    switch (command[0]) {
        case VIDEO_REGISTER:
            if (owner == -1) {
                owner = pid;
                rpc_send_dword_response(pid, correlation_id, 1);
            } else {
                rpc_send_dword_response(pid, correlation_id, 0);
            }
            break;
        case VIDEO_UNREGISTER:
            if (owner == pid) {
                owner = -1;
                rpc_send_dword_response(pid, correlation_id, 1);
            } else {
                rpc_send_dword_response(pid, correlation_id, 0);
            }
            break;
        case VIDEO_INIT:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            result = init_driver();
            rpc_send_dword_response(pid, correlation_id, result);
            break;
        case VIDEO_CLOSE:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            result = close_driver();
            rpc_send_dword_response(pid, correlation_id, result);
            break;
        case VIDEO_NEED_UPDATE:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
            }
            update_screen();
            rpc_send_dword_response(pid, correlation_id, 1);
            break;
        case VIDEO_NEED_PARTIAL_UPDATE:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
            }
            if (data_size < 5 * sizeof(int)) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            update_screen_partially(command[1], command[2], command[3],
                command[4]);
            rpc_send_dword_response(pid, correlation_id, 1);
            break;
        case VIDEO_SET_RESOLUTION:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            if (data_size < 4 * sizeof(int)) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            result = set_resolution(command[1],//Breite
                                    command[2],//Höhe
                                    command[3]);//bpp
            rpc_send_dword_response(pid, correlation_id, result);
            break;
        case VIDEO_GET_RESOLUTION:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            resolution_t resolution = get_resolution();
            rpc_send_response(pid, correlation_id, sizeof(resolution_t), (char*)&resolution);
            break;
        case VIDEO_GET_RESOLUTION_LIST:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            resolution_list_t reslist = get_possible_resolutions();
            int *tmpdata = malloc(reslist.res_count * sizeof(resolution_t) + 4);
            tmpdata[0] = reslist.res_count;
            memcpy(tmpdata + 1, reslist.resolutions, reslist.res_count * sizeof(resolution_t));
            rpc_send_response(pid, correlation_id, reslist.res_count * sizeof(resolution_t) + 4, (char*)tmpdata);
            if (reslist.res_count) {
                free(reslist.resolutions);
            }
            break;
        case VIDEO_GET_PALETTE:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            palette_t palette = get_palette();
            rpc_send_response(pid, correlation_id, sizeof(palette_t), (char*)&palette);
            if (palette.color_count) {
                free(palette.colors);
            }
            break;
        default:
            rpc_send_dword_response(pid, correlation_id, 0);
            break;
    }
    v();
    //printf("VGA-RPC Ende!\n");
}
