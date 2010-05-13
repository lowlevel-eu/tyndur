/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <rpc.h>
#include <init.h>
#include <collections.h>
#include <io.h>

#include "file.h"

#undef DEBUG

#define MOUNT_SRC_LEN 255
#define MOUNT_DST_LEN 255

#define MIN(a,b) (((a) > (b)) ? (b) : (a))

static list_t* mounts;

struct mount {
    size_t src_len;
    char src[MOUNT_SRC_LEN];
    char dst[MOUNT_DST_LEN];
};

static void rpc_io_open(pid_t pid, uint32_t cid, size_t data_size, void* data)
{
    uint32_t path_offset = 1 + sizeof(pid_t) + sizeof(io_resource_t);
    if (data_size < path_offset) {
        return;
    }

    char* path = (char*) ((uint32_t)data + path_offset);
    size_t path_len = data_size - path_offset;

#ifdef DEBUG
    printf("Zugriff auf file:%s\n", path);
#endif    

    io_resource_t io_res;
    io_res.pid = 0;

    int i;
    struct mount* item;
    for (i = 0; (item = list_get_element_at(mounts, i)); i++) {
        if (!strncmp(item->src, path, MIN(item->src_len, path_len))) {
            size_t dst_len = strlen(item->dst);
            char new_request[1 + path_len + dst_len + 1];
            char* new_path = &new_request[1];
            strncpy(new_path, item->dst, dst_len);
            strncpy(new_path + dst_len, path + item->src_len, path_len);
            new_path[path_len + dst_len] = '\0';

            // Pfade duerfen an der Nahtstelle keinen doppelten / enthalten
            if ((item->dst[dst_len - 1] == '/') 
                && (path[item->src_len] == '/')) 
            {
                memmove(new_path + dst_len, new_path + dst_len + 1, 
                    path_len);
            }

#ifdef DEBUG
            printf("Zugriff auf file:%s => %s\n", path, new_path);
#endif

            new_request[0] = ((char*) data)[0];

            response_t* resp = rpc_get_response(1, "IO_OPEN ", 
                strlen(new_path) + 2, new_request);
    
            io_resource_t* resp_file = (io_resource_t*) resp->data;
            if (resp->data_length && resp_file && resp_file->pid) {
                io_res = *resp_file;
            }
            free(resp->data);
            free(resp);

            break;
        }
    }
        
    rpc_send_response(pid, cid, sizeof(io_resource_t), (char*) &io_res);
}

static void rpc_mount(pid_t pid, uint32_t cid, size_t data_size, void* data)
{
    char* src = data;
    size_t src_size = strnlen(src, data_size);

    if (src_size == data_size) {
        return;
    }

    char* dst = data + src_size + 1;
    size_t dst_size = strnlen(dst, data_size - src_size - 1);
    
    if (dst_size == data_size) {
        return;
    }

    if (strncmp("file:", src, 5)) {
        return;
    }

    src += 5;
    src_size -= 5;

    printf("[  FILE  ] file:%s => %s\n", src, dst);

    struct mount* mount = malloc(sizeof(*mount));
    mount->src_len = src_size;
    strncpy(mount->src, src, MOUNT_SRC_LEN);
    strncpy(mount->dst, dst, MOUNT_DST_LEN);

    mount->src[MOUNT_SRC_LEN - 1] = '\0';
    mount->dst[MOUNT_DST_LEN - 1] = '\0';

    int i;
    struct mount* item;
    for (i = 0; (item = list_get_element_at(mounts, i)); i++) {
        if (item->src_len < src_size) {
            break;
        }
    }

    list_insert(mounts, i, mount);

#ifdef DEBUG
    printf("Dump mounts:\n");
    for (i = 0; (item = list_get_element_at(mounts, i)); i++) {
        printf("%s => %s\n", item->src, item->dst);
    }
#endif
}

int server(void)
{
    mounts = list_create();

    register_message_handler("IO_OPEN ", &rpc_io_open);
    register_message_handler("MOUNT", &rpc_mount);

    init_service_register(SERVICE_NAME);

    while (1) {
        wait_for_rpc();
    }

    return 0;
}
