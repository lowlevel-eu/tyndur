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
#include "tcp.h"

bool debug_checksum = FALSE;

word ip_checksum(void* data, size_t size)
{
    int i;
    dword checksum = 0;
    word* data_word = (word*) data;

    if (size % 2) {
        checksum += big_endian_word(((char*) data)[size - 1]);
    }

    if (debug_checksum) printf("Checksum = %x\n", checksum);
    for (i = 0; i < (size / 2); i++) {
        checksum += big_endian_word(data_word[i]);
        if (debug_checksum) printf("[%d] + %x = %x\n", i, big_endian_word(data_word[i]), checksum);
    }

    if (debug_checksum) printf("Checksum_komplett = %08x\n", checksum);
    checksum = (checksum & 0xFFFF) + (checksum >> 16);
    checksum = big_endian_word(~checksum);
    if (debug_checksum) printf("~ Checksum_komplett = %04x\n", checksum);

    return checksum;
}

void ip_update_checksum(struct ip_header* header)
{
    // Die Checksumme wird unter der Voraussetzung berechnet, daß der Wert
    // des Felds für die Checksumme 0 ist.
    header->checksum = 0;
    header->checksum = ip_checksum(header, 4 * header->ihl);
}

word tcp_checksum
    (struct tcp_header* header, struct tcp_pseudo_header* pseudo_header,
    void* data, dword data_size)
{
    //printf("data_size = %d\n", data_size);
    char buffer[sizeof(struct tcp_header) + sizeof(struct tcp_pseudo_header) 
        + data_size];

    memcpy(buffer, pseudo_header, sizeof(struct tcp_pseudo_header));
    memcpy(buffer + sizeof(struct tcp_pseudo_header), header, 
        sizeof(struct tcp_header));
    memcpy(buffer + sizeof(struct tcp_pseudo_header) + sizeof(struct tcp_header), 
        data, data_size);

    return ip_checksum(buffer, 
        sizeof(struct tcp_pseudo_header) + sizeof(struct tcp_header) +
        data_size);

}

void tcp_update_checksum
    (struct tcp_header* header, struct tcp_pseudo_header* pseudo_header,
    void* data, dword data_size)
{
    header->checksum = 0;
    header->checksum = tcp_checksum(header, pseudo_header, data, data_size);
}
