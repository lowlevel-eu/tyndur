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

#include "network.h"
#include "ip.h"


#define _USE_START_
#include "init.h"

void test_ping();
void init_routing();

int main(int argc, char* argv[])
{
    init_ip();
    init_routing();
    test_ping();
    
    init_service_register("tcpip");

    while(1) {
        wait_for_rpc();
    }
}

void init_routing()
{
    struct routing_entry entry = {
        .target = 0,
        .subnet = 0,
        .gateway = 0x0202000a,
        .driver = init_service_get("rtl8139"),
        .device = 0
    };

    struct routing_entry* pentry = malloc(sizeof(entry));
    *pentry = entry;

    add_routing_entry(pentry);
}

#if 0
void ip_calc_checksum(struct ip_header* header)
{
    int i;
    word* header_word = (word*) header;

    dword checksum = 0;

    // TODO header->ihl statt 5 (Vorsicht vor dem Pufferüberlauf)
    for (i = 0; i < 2*5; i++) {
        checksum += big_endian_word(header_word[i]);
    }

    checksum = (checksum & 0xFFFF) + (checksum >> 16);
    checksum = big_endian_word(~checksum);

    header->checksum = checksum;
}
#endif

void test_ping()
{
    dword ping_size = 3 * sizeof(dword);
    dword ping[3] = {
        0xFFF70008,
        0x00000000,
        0x00000000
    };

    struct ip_header ipheader = {
        .version = 4,
        .ihl = 5,
        .type_of_service = 0,
        .total_length = big_endian_word(sizeof(ipheader) + ping_size),
        .id = big_endian_word(1),
        .flags = 0,
        .fragment_offset = big_endian_word(0),
        .ttl = 255,
        .protocol = IP_PROTO_ICMP,
        .checksum = 0,
        .source_ip = 0x0b01a8c0,
        .dest_ip = 0x0201a8c0
    };

    ip_update_checksum(&ipheader);

    printf("tcpip: send ping\n");
    ip_send(&ipheader, ping, ping_size);
    printf("tcpip: sent ping\n");
}
