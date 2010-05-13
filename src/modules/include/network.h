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

#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <stdint.h>

#define RPC_ETHERNET_RECEIVE_PACKET "ETH_RECV"
#define RPC_ETHERNET_SEND_PACKET "ETH_SEND"
#define RPC_ETHERNET_REGISTER_RECEIVER "ETH_REG "

#define RPC_IP_REGISTER_DRIVER "REGDRV  "

#define big_endian_word(x) ((((x) & 0xFF) << 8) | (((x) & 0xFF00) >> 8))
#define big_endian_dword(x) \
    ((big_endian_word((x) & 0xFFFF) << 16) | \
    (big_endian_word((x) >> 16)))

struct net_device {
    uint32_t number;
    uint64_t mac;
};

uint32_t string_to_ip(const char* ip);
char* ip_to_string(uint32_t ip);
char* mac_to_string(uint64_t mac);
void register_netcard(uint32_t device_numer, uint64_t mac);

#endif
