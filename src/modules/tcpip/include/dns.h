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

#ifndef _DNS_H_
#define _DNS_H_

#include <stdint.h>

#define DNS_PORT 53

struct dns_header {
    uint16_t    id;
    uint16_t    flags;
    uint16_t    qd_count;
    uint16_t    an_count;
    uint16_t    ns_count;
    uint16_t    ar_count;
} __attribute__((packed));


// Konstanten für die Header-Flags

/// 0 = Query; 1 = Response
#define DNS_QR 0x80

/// Authoritative Answer? (Nur in Antwort)
#define DNS_AA 0x04

/// Truncation - Nachricht ist abgeschnitten
#define DNS_TC 0x02

/// Recursion Desired
#define DNS_RD 0x01

/// Recursion Available
#define DNS_RA 0x8000

/// Opcode aus einem DNS-Header
#define dns_get_opde(x) (((x) >> 3) & 0x0F)

/// Opcode in einem DNS-Header setzen
static inline void dns_set_opcode(struct dns_header* header, uint8_t opcode)
{
    header->flags &= ~0x78;
    header->flags |= (opcode << 3);
}

// Opcodes
#define DNS_OPCODE_QUERY    0x00
#define DNS_OPCODE_IQUERY   0x01
#define DNS_OPCODE_STATUS   0x02

// Response Codes
#define DNS_RCODE_OK        0x00
#define DNS_RCODE_EFORMAT   0x01
#define DNS_RCODE_ESERVER   0x02
#define DNS_RCODE_ENAME     0x03
#define DNS_RCODE_EIMPL     0x04
#define DNS_RCODE_EREFUSED  0x05

/// Reponse Code aus einem DNS-Header
#define dns_get_rcode(x) (((x) >> 8) & 0x0F)

#define QTYPE_A big_endian_word(1)
#define QCLASS_IN big_endian_word(1)

struct dns_request_result {
    uint32_t* ip_address;
    uint32_t ip_count;
};

struct dns_request_result* dns_request(char* domain);

#endif
