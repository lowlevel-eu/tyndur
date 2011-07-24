/*
 * Copyright (c) 2011 The tyndur Project. All rights reserved.
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

#ifndef UDP_H
#define UDP_H

#include "ip.h"

#include <stdint.h>
#include <stddef.h>

struct udp_header {
    uint16_t    src_port;
    uint16_t    dest_port;
    uint16_t    length;
    uint16_t    checksum;
} __attribute__ ((packed));

struct udp_pseudo_header {
    uint32_t    source_ip;
    uint32_t    dest_ip;
    uint16_t    proto;
    uint16_t    length;
} __attribute__ ((packed));

/**
 * Öffnet einen UDP-Clientsocket
 *
 * @param sport Lokale Portnummer
 * @param daddr IP-Adresse des Remote-Hosts
 * @param dport Portnummer des Remote-Hosts
 *
 * @return Der geöffnete Socket oder NULL bei Fehlern
 */
struct udp_socket* udp_open(uint16_t sport, uint32_t daddr, uint16_t dport);

/**
 * Öffnet einen UDP-Clientsocket über eine gegebene Route (ignoriert die
 * Routentabelle)
 *
 * @param route Route, die benutzt werden soll
 * @see udp_open
 */
struct udp_socket* udp_open_route(uint16_t sport, uint32_t daddr,
    uint16_t dport, struct routing_entry* route);

/**
 * Liest ein einzelnes UDP-Paket aus
 */
size_t udp_read(struct udp_socket* s, void* buf, size_t len);

/**
 * Schreibt ein einzelnes UDP-Paket
 */
size_t udp_write(struct udp_socket* s, void* buf, size_t len);

/**
 * Beendet eine UDP-Verbindung
 */
void udp_close(struct udp_socket *s);

void udp_register_lostio(void);
void udp_receive(uint32_t source_ip, void* data, uint32_t data_size);

uint16_t udp_checksum
    (struct udp_header* header, struct udp_pseudo_header* pseudo_header,
    void* data, uint32_t data_size);
void udp_update_checksum
    (struct udp_header* header, struct udp_pseudo_header* pseudo_header,
    void* data, uint32_t data_size);

#endif
