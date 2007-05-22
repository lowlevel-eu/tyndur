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

#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"
#include "collections.h"
#include "network.h"

#include "ip.h"

list_t* routing_table;

void init_ip()
{
    routing_table = list_create();
}

void add_routing_entry(struct routing_entry* entry)
{
    // TODO Prüfen, ob nicht ersetzt werden muß
    list_push(routing_table, entry);
}

struct routing_entry* get_routing_entry(dword ip)
{
    int i;
    struct routing_entry* entry;

    for (i = 0; (entry = list_get_element_at(routing_table, i)); i++)
    {
        if ((entry->target & entry->subnet) == (ip & entry->subnet)) {
            // TODO Bei mehreren zutreffenden Routen die spezifischere nehmen
            return entry;
        }
    }

    return NULL;
}

bool ip_send(struct ip_header* header, void* data, dword size) 
{
    struct routing_entry* route = get_routing_entry(header->dest_ip);

    if (route == NULL) {
        return FALSE;
    }
    
    struct net_send_header rpc_header = {
        .device     = route->device,
        .dest_ip    = route->gateway ? route->gateway : header->dest_ip
    };
    
    dword packet_size = size + sizeof(rpc_header) + sizeof(*header) + 8;
    char packet[packet_size];

    memcpy(packet, RPC_IP_SEND_PACKET, 8);
    memcpy(packet + 8, &rpc_header, sizeof(rpc_header));
    memcpy(packet + 8 + sizeof(rpc_header), header, sizeof(struct ip_header));
    memcpy(packet + 8 + sizeof(rpc_header) + sizeof(struct ip_header), data, size);

    
    /*
    printf("Treiber: %d\n", route->driver);
    {
        int i;
        for (i = 0; i < packet_size; i++) {
            printf("%02x ", ((char*) packet)[i]);
        }
        printf("\n");
    }
    */

    send_message(
        route->driver,
        512,
        0,
        packet_size,
        packet
    );

    return TRUE;
}
