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
#include <stdbool.h>
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"
#include "collections.h"
#include "network.h"
#include <errno.h>
#include <string.h>

#include "ip.h"
#include "tcp.h"

//Hier koennen die Debug-Nachrichten aktiviert werden
#define DEBUG 0

#if DEBUG == 1
    #define DEBUG_MSG(s) printf("[ TCPIP ] debug: %s() '%s'\n",__FUNCTION__,s)
    #define DEBUG_MSG1(x, y) \
        printf("[ TCPIP ] debug: %s() '", __FUNCTION__); \
        printf(x, y); \
        printf("'\n");
    #define DEBUG_MSG2(x, y, z) \
        printf("[ TCPIP ] debug: %s() '", __FUNCTION__); \
        printf(x, y, z); \
        printf("'\n");
#else
    #define DEBUG_MSG(s)
    #define DEBUG_MSG1(s,x)
    #define DEBUG_MSG2(s,x,y)
#endif

extern bool debug_checksum;

list_t* tcp_connections;
list_t* tcp_servers;

void tcp_send_ack(struct tcp_connection* conn);

void init_tcp()
{
    tcp_connections = list_create();
    tcp_servers = list_create();
}

static uint16_t tcp_get_free_port(void)
{
    // FIXME
    static uint16_t source_port = 32768;
    if (source_port > 65500) { //Bisschen Platz lassen
        source_port = 32768;
    }
    return source_port++;
}

/**
 * Gibt die MSS (Maximum Segment Size) fuer eine TCP-Verbindung zurueck, d.h.
 * die maximale Anzahl Bytes, die als Nutzdaten in einem TCP-Paket uebertragen
 * werden koennen
 */
size_t tcp_get_mss(struct tcp_connection* conn)
{
    return 1500 - sizeof(struct ip_header) - sizeof(struct tcp_header);
}

static void tcp_send_packet(struct tcp_connection* conn, void* data, 
    uint32_t data_size, uint8_t flags)
{
    uint32_t options_size = (flags & TCPF_SYN ? 4 : 0);

    struct tcp_header header = {
        .source_port    = big_endian_word(conn->source_port),
        .dest_port      = big_endian_word(conn->dest_port),
        .seq            = big_endian_dword(conn->seq),
        .ack            = big_endian_dword(conn->ack),
        .data_offset    = (sizeof(header) + options_size) / 4,
        .reserved       = 0,
        .flags          = flags,
        .window         = big_endian_word(conn->window),
        .checksum       = 0,
        .urgent_ptr     = 0
    };

    struct tcp_pseudo_header pseudo_header = {
        .source_ip      = conn->source_ip,
        .dest_ip        = conn->dest_ip,
        .proto          = big_endian_word(IP_PROTO_TCP),
        .length         = big_endian_word(sizeof(struct tcp_header) + data_size + options_size)
    };
    
    size_t buffer_size  = sizeof(header) + data_size + options_size;
    conn->seq += data_size;
    void* buffer = malloc(buffer_size);

    memcpy(buffer, &header, sizeof(header));
    if (flags & TCPF_SYN) {
        uint32_t options = 0x78050402;
        memcpy(buffer + sizeof(header), &options, sizeof(options));
    }
    memcpy(buffer + options_size + sizeof(header), data, data_size);

    tcp_update_checksum(buffer, &pseudo_header, buffer + sizeof(header), data_size + options_size);

    DEBUG_MSG2("Sending packet to %x, ack = %d", conn->dest_ip,
        conn->ack);
    ip_send(conn->dest_ip, IP_PROTO_TCP, buffer, buffer_size);
}

static void tcp_send_syn(struct tcp_connection* conn)
{
    conn->status = TCPS_WAIT_FOR_SYN_ACK;
    conn->seq = (uint32_t) get_tick_count();

    tcp_send_packet(conn, NULL, 0, TCPF_SYN);
    conn->seq++;
}

void tcp_send_ack(struct tcp_connection* conn)
{
    if (list_is_empty(conn->out_buffer)) {
        tcp_send_packet(conn, NULL, 0, TCPF_ACK);
    } else {
        struct tcp_out_buffer* data = list_pop(conn->out_buffer);
        DEBUG_MSG1("Sende %d Bytes", data->size);
        tcp_send_packet(conn, data->data, data->size, TCPF_ACK | TCPF_PSH);
        // TODO free
    }
}

static void tcp_send_syn_ack(struct tcp_connection* conn)
{
    conn->status = TCPS_CONNECTED;
    conn->seq = (uint32_t) get_tick_count();

    tcp_send_packet(conn, NULL, 0, TCPF_SYN | TCPF_ACK);
    conn->seq++;
}

static void tcp_send_reset(struct tcp_connection* conn)
{
    tcp_send_packet(conn, NULL, 0, TCPF_RST);
}

static void tcp_send_fin(struct tcp_connection* conn)
{
    // Ein FIN ist nur gueltig, wenn auch ein ACK gesendet wird
    tcp_send_packet(conn, NULL, 0, TCPF_FIN | TCPF_ACK);
    conn->seq++;
}

static void do_nothing(void) {}

struct tcp_connection* tcp_open_connection(uint32_t ip, uint16_t port)
{
    struct routing_entry* route = get_routing_entry(ip);
    uint64_t timeout;

    if (route == NULL) {
        return NULL;
    }

    p();
    struct tcp_connection* conn = malloc(sizeof(struct tcp_connection));

    conn->dest_ip = ip;
    conn->dest_port = port;
    conn->source_ip = route->device->ip;
    conn->source_port = tcp_get_free_port();
    conn->window = 0;
    conn->ack = 0;
    conn->out_buffer = list_create();
    conn->in_buffer = list_create();
    conn->to_lostio = list_create();

    list_push(tcp_connections, conn);

    DEBUG_MSG("tcp_send_syn");
    // FIXME
    v();
    tcp_send_syn(conn);

    DEBUG_MSG("Gesendet, warte auf Antwort.");
    timeout = get_tick_count() + 9 * 1000000;
    timer_register(do_nothing, 10 * 1000000);
    while ((conn->status == TCPS_WAIT_FOR_SYN_ACK)
        && (get_tick_count() < timeout))
    {
        wait_for_rpc();
    }

    if (conn->status == TCPS_WAIT_FOR_SYN_ACK) {
        tcp_send_reset(conn);
        tcp_free_connection(conn);
        DEBUG_MSG("Timeout beim Verbindungsaufbau");
        return NULL;
    }

    DEBUG_MSG("Verbindung hergestellt");

    return conn;
}

void tcp_free_connection(struct tcp_connection* conn)
{
    int tcp_list_size, i;

    tcp_list_size = list_size(tcp_connections);

    // Geht alle Elemente durch, es koennte ja sein, dass conn jetzt nicht mehr
    // das erste Element ist
    for (i = 0; i < tcp_list_size; i++) {
        if (list_get_element_at(tcp_connections, i) == conn) {
            p();
            // Falls inzwischen was geaendert wurde, dauert das erneute
            // Durchlaufen der ganze Liste relativ lang. Aber es passiert
            // vermutlich selten und anstatt die ganze Schleife zu locken, ist
            // das bestimmt besser.
            if (list_get_element_at(tcp_connections, i) != conn) {
                v();
                i = -1; // Wird nach dem continue auf 0 inkrementiert
                continue;
            }
            list_remove(tcp_connections, i);
            v();
            break;
        }
    }

    list_destroy(conn->out_buffer);
    list_destroy(conn->in_buffer);
    list_destroy(conn->to_lostio);
    free(conn);
}

void tcp_close_connection(struct tcp_connection* conn)
{
    uint64_t timeout;

    if (conn->status == TCPS_WAIT_FOR_LAST_ACK
        || conn->status == TCPS_WAIT_FOR_FIN_ACK
        || conn->status == TCPS_CLOSED)
    {
        DEBUG_MSG("Verbindung ist schon geschlossen");
    } else {
        DEBUG_MSG("tcp_send_fin_ack");
        conn->status = TCPS_WAIT_FOR_FIN_ACK;
        tcp_send_fin(conn);
    }

    timeout = get_tick_count() + 9 * 1000000;
    timer_register(do_nothing, 10 * 1000000);
    while ((conn->status != TCPS_CLOSED)
        && (get_tick_count() < timeout))
    {
        wait_for_rpc();
    }

    if (conn->status != TCPS_CLOSED) {
        tcp_send_reset(conn);
        DEBUG_MSG("Timeout beim Schliessen der Verbindung");
    }

    tcp_free_connection(conn);
}

void tcp_accept_connection(struct tcp_connection* conn)
{
    DEBUG_MSG("tcp_send_syn_ack");
    tcp_send_syn_ack(conn);
}


struct tcp_connection* tcp_get_connection
    (uint32_t remote_ip, uint16_t remote_port, uint16_t local_port)
{
    int i;
    struct tcp_connection* conn;

    for (i = 0; (conn = list_get_element_at(tcp_connections, i)); i++)
    {
        if ((conn->source_port  == local_port) &&  
            (conn->dest_port    == remote_port) &&
            (conn->dest_ip      == remote_ip))
        {
            return conn;
        }
    }

    return NULL;
}

uint32_t tcp_check_in_data_order(struct tcp_connection* conn)
{
    struct tcp_in_buffer* in_buffer;
    
    p();
    while ((in_buffer = list_get_element_at(conn->in_buffer, 0)))
    {
        if (in_buffer->seq == conn->ack) {
            DEBUG_MSG2("Lese Puffer %08x mit Groesse %d ein", 
                in_buffer->seq, in_buffer->size);
            list_pop(conn->in_buffer);
            conn->ack += in_buffer->size;
            
            /*
            DEBUG_MSG1("Payloadsize: %d", in_buffer->size);
            syscall_putsn(in_buffer->size, in_buffer->data);


            free(in_buffer->data);
            free(in_buffer);
            */
            list_insert
                (conn->to_lostio, list_size(conn->to_lostio), in_buffer);
            in_buffer->seq = 0;

        } else if (in_buffer->seq < conn->ack) {
            DEBUG_MSG1("Doppeltes Paket: %08x", in_buffer->seq);
            list_pop(conn->in_buffer);
        }else {
            DEBUG_MSG2("Erwartet: %08x; erstes: %08x", conn->ack,
                in_buffer->seq);
            break;
        }
    }
    v();

    DEBUG_MSG1("Verbleibende Segmente im in-Buffer: %d",
        list_size(conn->in_buffer));


    return list_size(conn->in_buffer);
}

void tcp_in_data
    (struct tcp_connection* conn, uint32_t seq, size_t size, void* data)
{
    struct tcp_in_buffer* in_buffer = malloc(sizeof(struct tcp_in_buffer));
    void* buffer_data = malloc(size);

    memcpy(buffer_data, data, size);

    in_buffer->seq  = seq;
    in_buffer->size = size;
    in_buffer->data = buffer_data;

    int i;
    struct tcp_in_buffer* old_buffer;
    p();
    for (i = 0; (old_buffer = list_get_element_at(conn->in_buffer, i)); i++) {
        if (in_buffer->seq < old_buffer->seq) {
            break;
        }
    }

    list_insert(conn->in_buffer, i, in_buffer);

    tcp_check_in_data_order(conn);
    v();
}

void tcp_receive(uint32_t source_ip, void* data, uint32_t data_size)
{
    struct tcp_header* header = data;
    struct tcp_connection* conn = tcp_get_connection(source_ip, 
        big_endian_word(header->source_port), 
        big_endian_word(header->dest_port));

    DEBUG_MSG("--");

    // Neue Verbindung fuer einen TCP-Server?
    if ((conn == NULL) && (header->flags == TCPF_SYN)) {
        int ret;
        ret = tcp_incoming_connection(source_ip, header, &conn);

        if (ret == -EEXIST) {
            // Doppeltes SYN-Paket ignorieren
            return;
        } else if (ret != 0) {
            // TODO Fehler, RST schicken
            return;
        }
    }

    if (conn == NULL) {
        DEBUG_MSG1("Verbindung nicht gefunden. IP = %08x", source_ip);
        return;
    }

    if (data_size < sizeof(struct tcp_header)) {
        DEBUG_MSG1("Paket mit zu kleiner Groesse: %d", data_size);
        return;
    }
    
    // Achtung: Die hier gesetzten Variablen z�hlen zum Payload auch noch
    // zus�tzliche Optionen im TCP-Header. Dies ist notwendig, damit diese
    // Optionen in die Checksummenberechnung einbezogen werden.
    // Weiter unten wird dann header->data_offset benutzt, um den tats�chlichen
    // Anfang der Daten zu bestimmen.
    void* payload = data + sizeof(struct tcp_header);
    uint32_t payload_size = data_size - sizeof(struct tcp_header);
    
    // Checksumme pr�fen
    struct tcp_pseudo_header pseudo_header = {
        .source_ip = source_ip,
        .dest_ip = conn->source_ip,
        .proto = big_endian_word(IP_PROTO_TCP),
        .length = big_endian_word(data_size)
    };

    DEBUG_MSG1("Headerlaenge: %d", header->data_offset);
    if (tcp_checksum(header, &pseudo_header, payload, payload_size)) {
        
        printf("\033[1;31mTCP-Checksumme ungueltig\n\033[0;37m\n\n");

#if 0
        debug_checksum = TRUE;
        p();
        printf("\n\nMIT CHECKSUMME (Soll = 0)\nPaket: %08x\n", header->checksum);
        printf("tcpip: %08x\nLaenge:%d\n", 
            tcp_checksum(header, &pseudo_header, payload, payload_size),
            data_size
        );
        header->checksum = 0;
        printf("\n\nOHNE CHECKSUMME:\nPaket: %08x\n", header->checksum);
        printf("tcpip: %08x\nLaenge:%d\n", 
            tcp_checksum(header, &pseudo_header, payload, payload_size),
            data_size
        );

        debug_checksum = FALSE;
        
        {
            int i;
            for (i = 0; i < data_size; i++) {
                printf("%02x ", ((char*) data)[i]);
            }
            printf("\n");
        }
        printf("\n");

        v();        
#endif        
        return;
    }
    
    // Hier brauchen wir dann die wirklich korrekten Werte f�r den Payload
    payload = data + (4 * header->data_offset);
    payload_size = data_size - (4 * header->data_offset);

    DEBUG_MSG1("Ack:   %08x", header->ack);
    DEBUG_MSG1("Seq:   %08x", header->seq);
    DEBUG_MSG1("Flags: %02x", header->flags);
    DEBUG_MSG1("Size:  %02x", payload_size);

    // FIXME Eigentlich m�ssen nicht nur die Daten in die richtige Reihenfolge
    // gebracht werden, sondern alle Pakete. Ansonsten k�nnte z.B. ein FIN vor
    // den Daten ankommen und daf�r sorgen, da� diese verworfen werden
    switch (conn->status) {
        case TCPS_WAIT_FOR_SYN_ACK:
            if ((header->flags & TCPF_SYN) && (header->flags & TCPF_ACK)) {
                DEBUG_MSG("SYN ACK erhalten");
                conn->ack    = big_endian_dword(header->seq) + 1;//data_size;
                conn->window = 0x1000;
                conn->status = TCPS_CONNECTED;
                tcp_send_ack(conn);
            } else {
                DEBUG_MSG("SYN ACK erwartet, was anderes erhalten");
                tcp_send_reset(conn);
            }
            break;

        case TCPS_CONNECTED:
        case TCPS_CONNECTED_FIN:
            DEBUG_MSG("TCP-Segment erhalten, Status verbunden.");
            //conn->ack    = big_endian_dword(header->seq) + payload_size;
            
            if (header->flags & TCPF_FIN) {
                DEBUG_MSG("FIN erhalten");
                conn->status = TCPS_CONNECTED_FIN;                
                conn->fin_seq = big_endian_dword(header->seq);
            }

            if (payload_size) {
                tcp_in_data(conn, big_endian_dword(header->seq), 
                    payload_size, payload);
                tcp_send_ack(conn);
                
                /*printf("Payloadsize: %d\n", payload_size);
                syscall_putsn(payload_size, payload);*/
            }
                
            if ((conn->status == TCPS_CONNECTED_FIN) && 
//                (tcp_check_in_data_order(conn) == 0)) 
                (conn->ack == conn->fin_seq))
            {
                conn->status = TCPS_WAIT_FOR_LAST_ACK;
                conn->ack++;
                DEBUG_MSG("Sende FIN ACK.");
                tcp_send_fin(conn);
            }


            /*{
                int i;
                for (i = 0; i < payload_size; i++) {
                    printf("%02x ", ((char*) payload)[i]);
                }
                printf("\n");
            }*/
            break;

        case TCPS_WAIT_FOR_FIN_ACK:
            if ((header->flags & TCPF_FIN) && (header->flags & TCPF_ACK)) {
                DEBUG_MSG("FIN ACK erhalten");
                conn->ack = big_endian_dword(header->seq) + 1;
                tcp_send_ack(conn);
                conn->status = TCPS_CLOSED;
            }
            break;

        case TCPS_WAIT_FOR_LAST_ACK:
            // TODO: Pr�fen, ob f�r das richtige Segment
            DEBUG_MSG("ACK");
            conn->status = TCPS_CLOSED;
            break;

        case TCPS_WAIT_FOR_ACCEPT:
        case TCPS_CLOSED:
            break;

        default:
            DEBUG_MSG1("Unerwartetes TCP-Segment im Status %d", conn->status);
            tcp_send_reset(conn);
            break;
    }

}
