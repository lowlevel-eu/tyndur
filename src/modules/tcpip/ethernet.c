/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Jörg Pfähler.
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
#include <types.h>
#include <string.h>

#include "ethernet.h"
#include "arp.h"
#include "ip.h"

#include "network.h"
#include "rpc.h"
#include "syscall.h"

static void rpc_ethernet_receive(pid_t pid, uint32_t cid, size_t data_size,
    void* data);

void init_ethernet()
{
    register_message_handler(RPC_ETHERNET_RECEIVE_PACKET, rpc_ethernet_receive);
}

void ethernet_send_packet(struct device *device,
                          void *packet,
                          size_t size)
{
    size_t rpc_size = 8 + sizeof(uint32_t) + size;
    char rpc_packet[rpc_size];
    memcpy(rpc_packet, RPC_ETHERNET_SEND_PACKET, 8);
    memcpy(rpc_packet + 8, &device->dev.number, sizeof(uint32_t));
    memcpy(rpc_packet + 8 + sizeof(uint32_t), packet, size);

    send_message(device->driver->pid,
                 512,
                 0,
                 rpc_size,
                 rpc_packet);
}

void rpc_ethernet_receive(pid_t pid, uint32_t cid, size_t data_size, void* data)
{
    if (data_size < (sizeof(uint32_t) + sizeof(struct eth_packet_header))) {
        return;
    }

    uint32_t device_number = * (uint32_t *)data;
    struct device *device = get_device_by_pid(pid, device_number);
    if (device == NULL)
        return;

    struct eth_packet_header* eth = (struct eth_packet_header*)
        (data + sizeof(uint32_t));
    void *subpacket = (void*)eth + sizeof(struct eth_packet_header);
    size_t subsize = data_size - sizeof(uint32_t) -
        sizeof(struct eth_packet_header);
    switch (eth->type) {
        case ETH_TYPE_IP:
            ip_receive(device, subpacket, subsize);
            break;

        case ETH_TYPE_ARP:
            arp_receive(device, subpacket, subsize);
            break;
    }
}
