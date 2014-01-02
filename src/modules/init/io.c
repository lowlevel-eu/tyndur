/*  
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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
#define MODULE_INIT

#include <stdint.h>
#include "types.h"
#include "syscall.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "io.h"
#include "lostio.h"
#include "rpc.h"
#include "init.h"



void io_init()
{
    register_message_handler("IO_OPEN ", &rpc_io_open);
}

io_resource_t* newio_open(const char* service_name, const char* path,
    uint8_t attr, pid_t pid)
{
    char* p;
    lio_resource_t r;
    lio_stream_t s;
    io_resource_t* result = NULL;

    // Nur fuer Pipequellen
    if (attr != (IO_OPEN_MODE_READ | IO_OPEN_MODE_WRITE)) {
        return NULL;
    }

    asprintf(&p, "%s:%s", service_name, path);
    r = lio_resource(p, 1);
    if (r < 0) {
        goto out;
    }

    s = lio_open(r, LIO_READ | LIO_WRITE);
    if (s < 0) {
        goto out;
    }

    result = malloc(sizeof(*result));
    *result = (io_resource_t) {
        .lio2_stream    =   s,
        .lio2_res       =   r,
        .flags          =   IO_RES_FLAG_READAHEAD,
    };

out:
    free(p);
    return result;
}


io_resource_t* io_open_res(const char* service_name, const char* path, io_resource_t* pipe_source, uint8_t attr, pid_t pid)
{
    io_resource_t* newres;
    if ((newres = newio_open(service_name, path, attr, pid))) {
        return newres;
    }
    struct service_s* dest_service = NULL;
    dest_service = get_service_by_name(service_name);
    //printf("\nopen resource: service='%s' path='%s' => %x\n", service_name, path, dest_service); 
    
    if(dest_service == NULL)
    {
        return NULL;
    }
    

    // Pipequelle für den Service lesbar machen
    if (pipe_source && IS_LIO2(pipe_source)) {
        pipe_source->lio2_stream =
            lio_pass_fd(pipe_source->lio2_stream, dest_service->pid);
        if (pipe_source->lio2_stream < 0) {
            return NULL;
        }
    }
    
    //Daten fuer den open-rpc an den Service zusammenflicken
    size_t open_request_size = strlen(path) + sizeof(io_resource_t) + sizeof(pid_t) + 6;
    uint8_t* open_request = malloc(open_request_size);
    io_resource_t* open_req_res = (io_resource_t*)((uint32_t) open_request + 1 + sizeof(pid_t));
    //Pipe-Handle kopieren falls vorhanden
    if(pipe_source != NULL)
    {
        memcpy(open_req_res, pipe_source, sizeof(io_resource_t));
    }
    else
    {
        memset(open_req_res, 0, sizeof(io_resource_t));
    }
    open_request[0] = attr;
    *((pid_t*) &(open_request[1])) = pid;

    memcpy((void*) ((uint32_t)open_request + sizeof(io_resource_t) + sizeof(pid_t) + 1), path, strlen(path) + 1);
    
    response_t* resp = rpc_get_response(dest_service->pid, "IO_OPEN ", open_request_size, (char*) open_request);
    return (io_resource_t*) resp->data;
}

void rpc_io_open(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    uint8_t* attr = (uint8_t*) data;
    char* path = (char*) ((uint32_t) data + 1);
    size_t path_length = strnlen(path, data_size - 1);
    char* service_name = NULL;
    io_resource_t* pipe_source = NULL;
    int i, last_pipe;
    pid_t last_pid = pid;
    //printf("[  INIT  ] rpc_io_open: path='%s', attr='%x'\n", path, *attr);
    
    // Pruefen, ob der Pfad nullterminiert ist
    if (path_length >= data_size - 1) {
        rpc_send_response(pid, correlation_id, 0, NULL);
        return;
    }

    //Hier werden die einzelnen Handles fuer die Pipes geholt
    last_pipe = 0;

    for(i = 0; i < path_length + 1; i++)
    {
        if((path[i] == '|') || (path[i] == '\0'))
        {
            //Das Pipe-Zeichen mit 0 ersetzen, damit strlen usw. in io_open_res nicht fehlschlagen
            path[i] = '\0';
            char* current_path = &(path[last_pipe]);
            //Den Service-Namen aus dem Pfad holen
            char* service_name_end = strstr(current_path, ":/");
            if(service_name_end != NULL)
            {
                size_t service_name_size = (uint32_t)service_name_end - (uint32_t)current_path;
                service_name = malloc(service_name_size + 1);
                strncpy(service_name, current_path, service_name_size);
                service_name[service_name_size] = '\0';
                
                //Bei der letzten Pipe werden die angegebenen Attribute uebergeben
                //FIXME: Das ist nur ein Bugfix
                if(i == path_length)
                {
                    pipe_source = io_open_res(service_name, (char*)((uint32_t)strstr(current_path, ":/") + 1), pipe_source, *attr, last_pid);
                }
                else
                {
                    pipe_source = io_open_res(service_name, (char*)((uint32_t)strstr(current_path, ":/") + 1), pipe_source, IO_OPEN_MODE_READ | IO_OPEN_MODE_WRITE, last_pid);
                }

                if((pipe_source != NULL) && pipe_source->pid)
                {
                    last_pid = pipe_source->pid;
                } else if (pipe_source && IS_LIO2(pipe_source)) {
                    /* pipe_source bleibt gültig, aber es gibt keine PID */
                } else {
                    pipe_source = NULL;
                    break;
                }
            }
            else
            {
                rpc_send_response(pid, correlation_id, 0, NULL);
                return;
            }

            last_pipe = i+1;
        }

    }
    
    if(pipe_source == NULL)
    {
        rpc_send_response(pid, correlation_id, 0, NULL);
    }
    else
    {
        rpc_send_response(pid, correlation_id, sizeof(io_resource_t), (char*) pipe_source);
    }

}


