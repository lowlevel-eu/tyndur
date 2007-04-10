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
#include "types.h"
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "rpc.h"
#include "io.h"
#include "ports.h"
#include "dma.h"

#define _USE_START_
#include "init.h"





void rpc_io_open(pid_t pid, dword correlation_id, size_t data_size, void* data);
void rpc_io_close(pid_t pid, dword correlation_id, size_t data_size, void* data);
void rpc_io_read(pid_t pid, dword correlation_id, size_t data_size, void* data);
void rpc_io_write(pid_t pid, dword correlation_id, size_t data_size, void* data);
void rpc_io_seek(pid_t pid, dword correlation_id, size_t data_size, void* data);




byte irq_count;
dma_transfer_t dma_transfers[8];


word dma_ports_page[]   = {0x87, 0x83, 0x81, 0x82, 0x8f, 0x8b, 0x89, 0x8a};
word dma_ports_addr[]   = {0x00, 0x02, 0x04, 0x06, 0xc0, 0xc4, 0xc8, 0xcc};
word dma_ports_count[]  = {0x01, 0x03, 0x05, 0x07, 0xc2, 0xc6, 0xca, 0xce};
word dma_ports_mask[]   = {0x0a, 0x0a, 0x0a, 0x0a, 0xd4, 0xd4, 0xd4, 0xd4};
word dma_ports_mode[]   = {0x0b, 0x0b, 0x0b, 0x0b, 0xd6, 0xd6, 0xd6, 0xd6};
word dma_ports_clear[]  = {0x0c, 0x0c, 0x0c, 0x0c, 0xd8, 0xd8, 0xd8, 0xd8};








int main(int argc, char* argv[])
{    
    register_message_handler("IO_OPEN ", &rpc_io_open);
    register_message_handler("IO_CLOSE", &rpc_io_close);
    register_message_handler("IO_READ ", &rpc_io_read);
    register_message_handler("IO_WRITE", &rpc_io_write);
    register_message_handler("IO_SEEK ", &rpc_io_seek);
    
    int i;
    for(i = 0; i < DMA_CHANNEL_COUNT; i++)
    {
        dma_transfers[i].used = FALSE;
    }
    

    init_service_register("dma");

    while(1) yield();
    return 0;
}


bool setup_dma(byte channel, byte mode, dword length)
{
    if(channel > (DMA_CHANNEL_COUNT - 1))
        return FALSE;
    
    
    dma_mem_ptr_t dma_ptr = mem_dma_allocate(length, 0x80);
    //printf("dma_ptr->virt:0x%08x  dma_ptr->phys:0x%08x\n", dma_ptr.virt, dma_ptr.phys);
 
    dma_transfers[channel].used = TRUE;
    dma_transfers[channel].data = dma_ptr;
    dma_transfers[channel].size = length;

    

    //Die Kannal-Bits in mode setzen
    mode = mode | (channel % 4);
   
    //Kanal maskieren
    outb(dma_ports_mask[channel], channel | 0x4);

    //Alle laufenden uebertragungen abbrechen
    outb(dma_ports_clear[channel], 0x0);

    //Modus senden
    outb(dma_ports_mode[channel], mode);
    
    //Adresse senden
    outb(dma_ports_addr[channel], (byte)((dword)(dma_transfers[channel].data.phys) & 0xff));
    outb(dma_ports_addr[channel], (byte)(((dword)(dma_transfers[channel].data.phys) >> 8) & 0xff));
    outb(dma_ports_page[channel], (byte)(((dword)(dma_transfers[channel].data.phys) >> 16) & 0xff));
    
    outb(dma_ports_clear[channel], 0x0);

    //Laenge senden
    length--;
    outb(dma_ports_count[channel], (byte)(length & 0xff));
    outb(dma_ports_count[channel], (byte)((length >> 8) & 0xff));

    //Start!
    outb(dma_ports_mask[channel], channel);

    return TRUE;
}

