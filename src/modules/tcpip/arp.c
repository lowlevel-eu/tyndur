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
#include <syscall.h>
#include <rpc.h>
#include <stdlib.h>
#include <collections.h>

#include "ethernet.h"
#include "arp.h"

struct arp_cache_entry {
    uint32_t   ip;
    uint64_t   mac : 48;
};

static list_t* arp_cache;

void init_arp()
{
    arp_cache = list_create();
}

static void arp_send_request(struct device *device,
                             uint32_t ip)
{
    uint32_t packet_size = sizeof(struct eth_packet_header) +
        sizeof(struct arp);
    void* packet = malloc(packet_size);

    struct eth_packet_header* eth = packet;
    struct arp* arp = packet + sizeof(struct eth_packet_header);
    
    eth->src = device->dev.mac;
    eth->dest = 0xFFFFFFFFFFFFLL;
    eth->type = ETH_TYPE_ARP;

    arp->hwtype = ARP_HW_ETHERNET;
    arp->proto = ARP_PROTO_IPV4;
    arp->hwaddrsize = ARP_HWADDRSIZE_ETHERNET;
    arp->protoaddrsize = ARP_PROTOADDRSIZE_IPV4;
    arp->command = ARP_CMD_REQUEST;

    arp->source_mac = device->dev.mac;
    arp->dest_mac = eth->dest;

    arp->source_ip = device->ip;
    arp->dest_ip = ip;

    ethernet_send_packet(device,
                         packet,
                         packet_size);

    free(packet);
}

void arp_send_response(struct device* device, struct arp* request) 
{
    uint32_t packet_size = sizeof(struct eth_packet_header) +
        sizeof(struct arp);
    void* packet = malloc(packet_size);

    struct eth_packet_header* eth = packet;
    struct arp* arp = packet + sizeof(struct eth_packet_header);
    
    eth->src = device->dev.mac;
    eth->dest = request->source_mac;
    eth->type = ETH_TYPE_ARP;

    arp->hwtype = ARP_HW_ETHERNET;
    arp->proto = ARP_PROTO_IPV4;
    arp->hwaddrsize = ARP_HWADDRSIZE_ETHERNET;
    arp->protoaddrsize = ARP_PROTOADDRSIZE_IPV4;
    arp->command = ARP_CMD_RESPONSE;

    arp->source_mac = eth->src;
    arp->dest_mac = eth->dest;

    arp->source_ip = device->ip;
    arp->dest_ip = request->source_ip;

    ethernet_send_packet(device,
                         packet,
                         packet_size);

    free(packet);
}

static uint64_t get_from_arp_cache(uint32_t ip)
{
    int i;
    struct arp_cache_entry* entry;

    p();
    for (i = 0; (entry = list_get_element_at(arp_cache, i)); i++) {
        if (entry->ip == ip) {
            v();
            return entry->mac;
        }
    }
    v();

    return 0;
}

uint64_t arp_resolve(struct device *device,
                  uint32_t ip)
{
    uint64_t mac;

    // Limited Broadcast muss an FF:FF... gehen
    if (ip == 0xFFFFFFFF) {
        return 0xFFFFFFFFFFFFLL;
    }

    if ((mac = get_from_arp_cache(ip)) == 0) 
    {
        arp_send_request(device, ip);

        while ((mac = get_from_arp_cache(ip)) == 0) {
            wait_for_rpc();
        }
    }

    return mac;
}

void arp_receive(struct device *device, void* packet, size_t size)
{
    struct arp* arp = packet;

    if (arp->command == ARP_CMD_RESPONSE) 
    {
        // TODO Falls der Eintrag schon drin ist, ignorieren bzw. updaten
        struct arp_cache_entry* entry = malloc(sizeof(struct arp_cache_entry));
        entry->ip   = arp->source_ip;
        entry->mac  = arp->source_mac;
        
        p();
        list_push(arp_cache, entry);
        v();
    }
    else if (arp->command == ARP_CMD_REQUEST)
    {
        if (arp->dest_ip == device->ip) {
            arp_send_response(device, arp);
        }
    }
}
