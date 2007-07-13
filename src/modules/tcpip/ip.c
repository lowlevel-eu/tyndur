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
#include "collections.h"

#include "network.h"
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
    #define DEBUG_MSG1(s, y)
    #define DEBUG_MSG2(s, y, z)
#endif

void rpc_ip_receive(pid_t pid, dword cid, size_t data_size, void* data);

list_t* routing_table;


void init_ip()
{
    routing_table = list_create();
    register_message_handler(RPC_IP_RECEIVE_PACKET, rpc_ip_receive);
}

void add_routing_entry(struct routing_entry* entry)
{
    // TODO Prüfen, ob nicht ersetzt werden muß
    list_push(routing_table, entry);
}

struct routing_entry* get_routing_entry(dword ip)
{
    int i;
    struct routing_entry* entry;

    for (i = 0; (entry = list_get_element_at(routing_table, i)); i++)
    {
        if ((entry->target & entry->subnet) == (ip & entry->subnet)) {
            // TODO Bei mehreren zutreffenden Routen die spezifischere nehmen
            return entry;
        }
    }

    return NULL;
}

bool ip_send_packet(struct ip_header* header, void* data, dword size) 
{
    struct routing_entry* route = get_routing_entry(header->dest_ip);

    if (route == NULL) {
        return FALSE;
    }

    header->source_ip = route->localip;
    ip_update_checksum(header);

    
    struct net_send_header rpc_header = {
        .device     = route->device,
        .dest_ip    = route->gateway ? route->gateway : header->dest_ip
    };
    
    dword packet_size = size + sizeof(rpc_header) + sizeof(*header) + 8;
    char packet[packet_size];

    memcpy(packet, RPC_IP_SEND_PACKET, 8);
    memcpy(packet + 8, &rpc_header, sizeof(rpc_header));
    memcpy(packet + 8 + sizeof(rpc_header), header, sizeof(struct ip_header));
    memcpy(packet + 8 + sizeof(rpc_header) + sizeof(struct ip_header), data, size);

    
    //DEBUG_MSG1("Treiber: %d", route->driver);
    /*
    {
        int i;
        for (i = 0; i < packet_size; i++) {
            printf("%02x ", ((char*) packet)[i]);
        }
        printf("\n");
    }
    */

    send_message(
        route->driver,
        512,
        0,
        packet_size,
        packet
    );

    return TRUE;
}

bool ip_send(dword dest_ip, byte proto, void* data, size_t data_size)
{
    struct ip_header ip_header = {
        .version = 4,
        .ihl = 5,
        .type_of_service = 0,
        .total_length = big_endian_word(sizeof(ip_header) + data_size),
        .id = big_endian_word(1),
        .fragment_offset = big_endian_word(0),
        .ttl = 255,
        .protocol = proto,
        .checksum = 0,
        .source_ip = 0,
        .dest_ip = dest_ip 
    };

    return ip_send_packet(&ip_header, data, data_size);
}

void rpc_ip_receive(pid_t pid, dword cid, size_t data_size, void* data)
{
    struct ip_header* header = data;

    if (data_size < sizeof(struct ip_header)) {
        DEBUG_MSG1("Paket mit zu kleiner Groesse: %d", data_size);
        return;
    }

    word total_length = big_endian_word(header->total_length);
    byte ihl = header->ihl;

    // Wenn data_size größer als total_length ist, darf das Paket nicht
    // verworfen werden, da Ethernetpakete eine Mindestgröße haben und
    // daher größer sein können als es von IP aus eigentlich nötig wäre
    if (data_size < total_length) {
        DEBUG_MSG2("Paket mit falscher Groesse im IP-Header "
            "(Header: %d; Tatsaechlich: %d", total_length, data_size);
        return;
    }

    if ((ihl * 4) > total_length) {
        DEBUG_MSG2("Header laenger (%d) als Paket (%d)", 
            ihl * 4, total_length);
        return;
    }

    if (ip_checksum(header, ihl * 4) != 0) {
        DEBUG_MSG("Header-Checksumme ungueltig");
        return;
    }

    // TODO Prüfen, ob das Paket überhaupt für uns bestimmt ist
    // Ansonsten sollte man es theoretisch weiterleiten
    
    // Wenn entweder das Flag IP_FLAG_MORE_FRAGMENTS gesetzt oder
    // die Fragmentnummer größer als Null ist, muss zusammengesetzt
    // werden.
    if (big_endian_word(header->fragment_offset) & ~IP_FLAG_DONT_FRAGMENT) {
        DEBUG_MSG("Fragmentiertes Paket");
        return;
    }
    
    DEBUG_MSG("rpc_ip_receive");
    DEBUG_MSG1("data_size: %08x", data_size);
    DEBUG_MSG1("Source-IP: %08x", header->source_ip);
    DEBUG_MSG1("Dest-IP:   %08x", header->dest_ip);

    void* packet = data + (ihl * 4);
    dword packet_size = total_length - (ihl * 4);
    
    switch(header->protocol) { 
        case IP_PROTO_ICMP:
            DEBUG_MSG("ICMP-Paket eingegangen");
            break;

        case IP_PROTO_TCP:
            DEBUG_MSG("TCP-Paket eingegangen");
            tcp_receive(header->source_ip, packet, packet_size);
            break;

        case IP_PROTO_UDP:
            DEBUG_MSG("UDP-Paket eingegangen");
            break;

        default:
            DEBUG_MSG1("Unbekanntes Protokoll %d", header->protocol);
            break;
    }
}
