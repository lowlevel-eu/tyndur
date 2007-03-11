/*  
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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

#include "stdio.h"
#include "types.h"
#include "ports.h"
#include "stdlib.h"
#include "io.h"
#include "collections.h"
#include "lostio.h"

#include "pci.h"

#define _USE_START_
#include "init.h"

struct pci_filehandle {
    io_resource_id_t res_id;
    char* buffer;
    size_t len;
    dword pos;
    dword flags;
};

//Die Handler fuer die LostIO-Schnittstelle
void    rpc_io_open(pid_t pid, dword correlation_id, size_t data_size, void* data);
void    rpc_io_close(pid_t pid, dword correlation_id, size_t data_size, void* data);
void    rpc_io_read(pid_t pid, dword correlation_id, size_t data_size, void* data);
void    rpc_io_write(pid_t pid, dword correlation_id, size_t data_size, void* data);
void    rpc_io_seek(pid_t pid, dword correlation_id, size_t data_size, void* data);
void    rpc_io_eof(pid_t pid, dword correlation_id, size_t data_size, void* data);

static list_t* filehandles;
static io_resource_id_t cur_res_id = 0;

int main(int argc, char* argv[])
{
    filehandles = list_create();

    //Handler fuer LostIO initialisieren
    register_message_handler("IO_OPEN ", &rpc_io_open);
    register_message_handler("IO_CLOSE", &rpc_io_close);
    register_message_handler("IO_READ ", &rpc_io_read);
    register_message_handler("IO_WRITE", &rpc_io_write);
    register_message_handler("IO_SEEK ", &rpc_io_seek);
    register_message_handler("IO_EOF  ", &rpc_io_eof);
    
    init_service_register("pci");

    while(1) {
        yield();
    }

    return 0;
}


int get_filehandle_index(io_resource_id_t res_id)
{
    struct pci_filehandle* cur_handle;
    int index = 0;

    while ((cur_handle = (struct pci_filehandle*) list_get_element_at(filehandles, index))) {
        if (cur_handle->res_id == res_id) {
            return index;
        }
        index++;
    }

    return -1;
}

/**
 * Wird aufgerufen, wenn ein Prozess ein fopen fuer irgendeine Datei in floppy:/
 * ausfuehrt.
 */
void rpc_io_open(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    io_resource_t io_res;
    struct pci_filehandle* fileinfo;
    
/*    byte* attr = (byte*) data;
    char* path = (char*) ((dword)data + 1 + sizeof(io_resource_t));*/

    io_res.pid = init_service_get("pci");
    if (1) {
        io_res.id = cur_res_id++;

        fileinfo = malloc(sizeof(struct pci_filehandle));

        fileinfo->res_id = io_res.id;
        fileinfo->buffer = pciinfo();
        fileinfo->pos = 0;
        fileinfo->len = strlen(fileinfo->buffer);

        p();
        list_push(filehandles, fileinfo);
        v();
    } else {
        // Verzeichnis
    }
        
    rpc_send_response(pid, correlation_id, sizeof(io_resource_t), (char*) &io_res);
}


/**
 * Wird aufgerufen, wenn ein Prozess ein fclose mit einem File-Handle vom
 * aktuellen Treiber als Parameter aufruft.
 */
void rpc_io_close(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    int index;
    io_resource_t* io_res = (io_resource_t*) data;
    
    p();
    index = get_filehandle_index(io_res->id);
    if (index != -1) {
        struct pci_filehandle* fileinfo = list_get_element_at(filehandles, index);
        free(fileinfo->buffer);
        free(fileinfo);
        list_remove(filehandles, index);
    }
    v();

    
    rpc_send_dword_response(pid, correlation_id, 0);
}


/**
 * Ein Prozess hat ein fread auf einem File-Handle dieses Treibers ausgefuehrt
 */
void rpc_io_read(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    int index;
    struct pci_filehandle* fileinfo = NULL;
    io_read_request_t* read_request = (io_read_request_t*) data;

    p();
    index = get_filehandle_index(read_request->id);
    if (index != -1) {
        fileinfo = list_get_element_at(filehandles, index);
    }
    v();

    if (fileinfo == NULL) {
        rpc_send_response(pid, correlation_id, 0, NULL);
    }

    size_t size = read_request->blocksize * read_request->blockcount;
    if (fileinfo->pos + size > fileinfo->len) {
        size = fileinfo->len - fileinfo->pos;
    }
        
    rpc_send_response(pid, correlation_id, size, &fileinfo->buffer[fileinfo->pos]);
    fileinfo->pos += size;
    if(fileinfo->pos == fileinfo->len)
    {
        fileinfo->flags |= LOSTIO_FLAG_EOF;
    }
    
}


/**
 * Ein Prozess hat ein fwrite auf einem File-Handle dieses Treibers ausgefuehrt
 */
void rpc_io_write(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    rpc_send_dword_response(pid, correlation_id, 0);
}


/**
 * Ein Prozess hat ein fseek auf einem File-Handle dieses Treibers ausgefuehrt
 */
void rpc_io_seek(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    rpc_send_dword_response(pid, correlation_id, 0);
}


void rpc_io_eof(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    io_eof_request_t* eof_request = (io_eof_request_t*) data;

    
    struct pci_filehandle* fileinfo = NULL;
    int index = get_filehandle_index(eof_request->id);
    if (index != -1) {
        fileinfo = list_get_element_at(filehandles, index);
    }
    
    if (fileinfo == NULL) {
            rpc_send_dword_response(pid, correlation_id, EOF);
            return;
    }

    if((fileinfo->flags & LOSTIO_FLAG_EOF) != 0)
    {
        rpc_send_dword_response(pid, correlation_id, EOF);
    }
    else
    {
        rpc_send_dword_response(pid, correlation_id, 0);
    }
}

