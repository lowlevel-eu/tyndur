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

#include "rtl8139.h"
#include "ethernet.h"
#include "arp.h"

list_t* ip_packet_receivers;

void rpc_send_packet(pid_t pid, dword cid, size_t data_size, void* data);
void rpc_register_receiver(pid_t pid, dword cid, size_t data_size, void* data);

void ip_init()
{
   register_message_handler(RPC_IP_SEND_PACKET, rpc_send_packet);
   register_message_handler(RPC_IP_REGISTER_RECEIVER, rpc_register_receiver);

   ip_packet_receivers = list_create();
}

bool ip_send_packet (struct netcard* netcard, dword ip, void* data, dword size)
{
    qword mac = arp_resolve(netcard, ip);
    
    dword packet_size = sizeof(struct eth_packet_header) + size;

    void* packet = malloc(packet_size);
    struct eth_packet_header* eth = packet;

    if (mac == 0) {
        return FALSE;
    }

    memcpy(packet + sizeof(struct eth_packet_header), data, size);
    
    eth->src = netcard->mac;
    eth->dest = mac;
    eth->type = ETH_TYPE_IP;

    send(netcard, packet, packet_size);

    free(packet);
    return TRUE;
}

void ip_receive(struct netcard* netcard, void* packet, size_t size)
{
    int i = 0;
    pid_t pid;

    if (size < sizeof(struct eth_packet_header)) {
        printf("Ethernet-Paket zu klein (%d)\n", size);
        return;
    }

    dword len = size + 8 - sizeof(struct eth_packet_header);

    char buffer[len];
    void* data = buffer;

    memcpy(data, RPC_IP_RECEIVE_PACKET, 8);
    memcpy(data + 8, packet + sizeof(struct eth_packet_header), 
        size - sizeof(struct eth_packet_header));

/*    printf("ip_receive %08x\n\033[1;36m", *((dword*)(data + len - 4)));
    
    {
        int i;
        for (i = 0; i < len; i++) {
            printf("%c", buffer[i]);
        }
        printf("\033[0;41m\n-------------\033[40;37m\n");
    }*/
    
    for (; (pid = (pid_t) list_get_element_at(ip_packet_receivers, i)); i++)
    {
        send_message(pid, 512, 0, len, data);
    }
    //printf("ip_receive sent\n");
}

void rpc_send_packet(pid_t pid, dword cid, size_t data_size, void* data)
{
    if (data_size < 8) {
        return;
    }

    struct net_send_header* header = data;
    void* packet = data + sizeof(struct net_send_header);

    ip_send_packet(the_one_and_only_netcard, header->dest_ip, 
        packet, data_size - sizeof(struct net_send_header));
}

void rpc_register_receiver(pid_t pid, dword cid, size_t data_size, void* data)
{
    list_push(ip_packet_receivers, (void*) pid);
}
