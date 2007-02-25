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
#define MODULE_INIT

#include "types.h"
#include "syscall.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "io.h"
#include "rpc.h"
#include "init.h"



void io_init()
{
    register_message_handler("IO_OPEN ", &rpc_io_open);
}


void rpc_io_open(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    byte* attr = (byte*) data;
    char* path = (char*) ((dword) data + 1);
    struct service_s* dest_service = get_service_by_name(path);
    
    if(dest_service == NULL)
    {
        rpc_send_dword_response(pid, correlation_id, 0);
        return;
    }

    
    //Daten fuer den open-rpc an den Service zusammenflicken
    size_t open_request_size = strlen(path) + sizeof(io_resource_t) + 6;
    byte* open_request = malloc(open_request_size);
    io_resource_t* open_req_res = (io_resource_t*)((dword) open_request + 1);
    open_request[0] = *attr;
    open_req_res->pid = 0;
    memcpy((void*) ((dword)open_request + sizeof(io_resource_t) + 1), path, strlen(path) + 1);
    
    
 
    response_t* resp = rpc_get_response(get_service_by_name(path)->pid, "IO_OPEN ", open_request_size, (char*) open_request);
    open_req_res = (io_resource_t*) resp->data;
    rpc_send_response(pid, correlation_id, sizeof(io_resource_t), (char*) open_req_res);
}


