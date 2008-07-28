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

#ifndef _IP_H__
#define _IP_H

#include "main.h"

#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

struct routing_entry {
    dword   target;
    dword   subnet;
    dword   gateway;
    struct device *device;
};

struct ip_header {
    // Nibbles vertauscht, weil Big Endian
    byte    ihl : 4;
    byte    version : 4;

    byte    type_of_service;
    word    total_length;
    word    id;

    word    fragment_offset;

    byte    ttl;
    byte    protocol;
    word    checksum;
    dword   source_ip;
    dword   dest_ip;
} __attribute__ ((packed));

#define IP_FLAG_MORE_FRAGMENTS (1 << 13)
#define IP_FLAG_DONT_FRAGMENT (1 << 14)

void init_ip();

void add_routing_entry(struct routing_entry* entry);
struct routing_entry* get_routing_entry(dword ip);

bool ip_send(dword dest_ip, byte proto, void* data, size_t data_size);
bool ip_send_packet(struct ip_header* header, void* data, dword size);

void ip_receive(struct device *device, void* data, size_t data_size);

word ip_checksum(void* data, size_t size);
void ip_update_checksum(struct ip_header* header);

#endif
