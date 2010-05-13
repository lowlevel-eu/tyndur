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

#ifndef _TCP_H_
#define _TCP_H_

#include <stdint.h>
#include "collections.h"

#define TCPS_WAIT_FOR_SYN_ACK 1
#define TCPS_CONNECTED 2
#define TCPS_WAIT_FOR_LAST_ACK 3
#define TCPS_CONNECTED_FIN 4
#define TCPS_CLOSED 5
#define TCPS_WAIT_FOR_ACCEPT 6
#define TCPS_WAIT_FOR_FIN_ACK 7

#define TCPF_FIN    (1 << 0)
#define TCPF_SYN    (1 << 1)
#define TCPF_RST    (1 << 2)
#define TCPF_PSH    (1 << 3)
#define TCPF_ACK    (1 << 4)
#define TCPF_URG    (1 << 5)

struct tcp_header {
    uint16_t    source_port;
    uint16_t    dest_port;
    uint32_t    seq;
    uint32_t    ack;

    uint8_t     reserved : 4;
    uint8_t     data_offset : 4;

    uint8_t     flags;
    uint16_t    window;
    uint16_t    checksum;
    uint16_t    urgent_ptr;
} __attribute__ ((packed));

struct tcp_pseudo_header {
    uint32_t    source_ip;
    uint32_t    dest_ip;
    uint16_t    proto;
    uint16_t    length;
} __attribute__ ((packed));

struct tcp_connection {
    uint32_t    source_ip;
    uint32_t    dest_ip;

    uint16_t    source_port;
    uint16_t    dest_port;

    uint32_t    seq;
    uint32_t    ack;

    list_t*     out_buffer;
    list_t*     in_buffer;

    list_t*     to_lostio;

    uint32_t    fin_seq;

    uint16_t    window;
    uint16_t    status;

    uint16_t    mss;
};

struct tcp_in_buffer {
    uint32_t    seq;
    size_t      size;
    void*       data;
};

struct tcp_out_buffer {
    size_t  size;
    void*   data;
};

struct tcp_server {
    uint16_t    port;
    list_t*     requests;
};


void init_tcp(void);
struct tcp_connection* tcp_open_connection(uint32_t ip, uint16_t port);
void tcp_close_connection(struct tcp_connection* conn);
void tcp_free_connection(struct tcp_connection* conn);
void tcp_accept_connection(struct tcp_connection* conn);

uint16_t tcp_checksum
    (struct tcp_header* header, struct tcp_pseudo_header* pseudo_header,
    void* data, uint32_t data_size);
void tcp_update_checksum
    (struct tcp_header* header, struct tcp_pseudo_header* pseudo_header,
    void* data, uint32_t data_size);

void tcp_receive(uint32_t source_ip, void* data, uint32_t data_size);
void tcp_send_ack(struct tcp_connection* conn); // TODO Ersetzen

/**
 * Gibt die MSS (Maximum Segment Size) fuer eine TCP-Verbindung zurueck, d.h.
 * die maximale Anzahl Bytes, die als Nutzdaten in einem TCP-Paket uebertragen
 * werden koennen
 */
size_t tcp_get_mss(struct tcp_connection* conn);

/**
 * Registriert die LostIO-Typehandles, die fuer TCP-Server benoetigt werden
 */
void tcp_server_register_lostio(void);


/**
 * Wird aufgerufen, um ein SYN-Paket zu behandeln. Erstellt eine Verbindung,
 * ohne jedoch SYN ACK zu senden. Stattdessen wird der Server benachrichtigt
 * und eine LostIO-Ressource angelegt, ueber die die Verbindung tatsaechlich
 * auf TCP-Ebene geoeffnet wird.
 *
 * Rueckgabewert:
 *   0          - Erfolg, die neue Verbindung wurde in *conn gespeichert
 *  -EEXIST     - Doppeltes SYN-Paket, die Verbindung existiert schon
 *  -ENOENT     - Kein Server lauscht auf dem Port
 *  -EIO        - Allgemeiner Fehler
 *  -ETIMEDOUT  - Verbindung wurde nicht angenommen
 */
int tcp_incoming_connection(uint32_t remote_ip,
    struct tcp_header* header, struct tcp_connection** res_conn);

#endif
