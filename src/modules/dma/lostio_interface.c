/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
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

#include <stdint.h>
#include "types.h"
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "rpc.h"
#include "io.h"
#include "init.h"
#include "dma.h"


void rpc_io_open(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    io_resource_t io_res;
    memset(&io_res, 0, sizeof(io_resource_t));
    uint32_t i;
    uint32_t nums[3];
    uint32_t num_id = 0;
    
    //Modus zu oeffnen
    //byte* attr = (byte*) data;
    char* path = (char*) ((uint32_t)data + 1 + sizeof(pid_t) + sizeof(io_resource_t));
    
    //Die Zahlen der einzelnen Pfad-Teile in dwords umwandeln
    for(i = 0; i < strlen(path); i++)
    {
        if(path[i] == '/')
        {
            nums[num_id++] = atoi(&(path[i+1]));    
            if(num_id >= 3)
            {
                break;
            }
        }
    }

    uint8_t channel = nums[0];
    uint8_t mode = nums[1];
    uint32_t size = nums[2];

    
    io_res.pid = get_pid();
    io_res.id = channel;
    
    dma_transfers[channel].pos = 0;
    
    setup_dma(channel, mode, size);
     
    rpc_send_response(pid, correlation_id, sizeof(io_resource_t), (char*) &io_res);
}


void rpc_io_close(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    io_resource_t* io_res = (io_resource_t*) data;

    free_dma(io_res->id);

    rpc_send_dword_response(pid, correlation_id, 0);
}


void rpc_io_read(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    io_read_request_t* read_request = (io_read_request_t*) data;

    // FIXME Hier sollte was richtiges implementiert werden, was auch eine 
    // Position im Puffer verwalten kann
    if (read_request->shared_mem_id == 0) {
        rpc_send_response(pid, correlation_id, 
            dma_transfers[read_request->id].size,
            (char*)(dma_transfers[read_request->id].data.virt));
    } else {
        // Shared Mem oeffnen, Daten reinkopieren und wieder schliessen
        void* shm_ptr = open_shared_memory(read_request->shared_mem_id);
        memcpy(shm_ptr, dma_transfers[read_request->id].data.virt,
            dma_transfers[read_request->id].size);
        close_shared_memory(read_request->shared_mem_id);

        rpc_send_response(pid, correlation_id,
            sizeof(dma_transfers[read_request->id].size),
            (void*) &dma_transfers[read_request->id].size);
    }
}


void rpc_io_write(pid_t pid, uint32_t correlation_id, size_t data_size,
    void* data)
{
    io_write_request_t* write_request = (io_write_request_t*) data;
    size_t max_size = dma_transfers[write_request->id].size - dma_transfers[write_request->id].pos;
    
    //Wenn der Cursor schon am Dateiende ist, kann nichtmehr geschrieben werden
    if(max_size == 0)
    {
        rpc_send_dword_response(pid, correlation_id, 0);
    }
    else
    {
       size_t size = write_request->blocksize * write_request->blockcount;
       if(size > max_size)
       {
            size = max_size;
       }

       memcpy(dma_transfers[write_request->id].data.virt, write_request->data, size);
       rpc_send_dword_response(pid, correlation_id, size);
    }
}



void rpc_io_seek(pid_t pid, uint32_t correlation_id, size_t data_size,
    void* data)
{
    io_seek_request_t* seek_request = (io_seek_request_t*) data;
    switch(seek_request->origin)
    {
        case SEEK_SET:
        {
            dma_transfers[seek_request->id].pos = seek_request->offset;
            break;
        }

        case SEEK_CUR:
        {
            dma_transfers[seek_request->id].pos += seek_request->offset;
            break;
        }
        
        case SEEK_END:
        {
            dma_transfers[seek_request->id].pos = dma_transfers[seek_request->id].size;
            break;
        }
    }

    if(dma_transfers[seek_request->id].pos > dma_transfers[seek_request->id].size)
    {
        dma_transfers[seek_request->id].pos = dma_transfers[seek_request->id].size;
    }
    rpc_send_dword_response(pid, correlation_id, 0);
}

