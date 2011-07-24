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
#include <string.h>
#include "collections.h"

#include "network.h"
#include "ip.h"
#include "arp.h"
#include "ethernet.h"
#include "tcp.h"
#include "udp.h"

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

list_t* routing_table;


void init_ip()
{
    routing_table = list_create();
}

void add_routing_entry(struct routing_entry* entry)
{
    // TODO Pr�fen, ob nicht ersetzt werden mu�
    list_push(routing_table, entry);
}

struct routing_entry* get_routing_entry(uint32_t ip)
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

struct routing_entry* enum_routing_entry(size_t i)
{
    if (i >= routing_get_entry_count())
        return NULL;

    return list_get_element_at(routing_table, i);
}

size_t routing_get_entry_count()
{
    return list_size(routing_table);
}

void remove_device_routes(struct device* device)
{
    int i;
    struct routing_entry* entry;

    for (i = 0; (entry = list_get_element_at(routing_table, i)); i++)
    {
        if (entry->device == device) {
            list_remove(routing_table, i);
            free(entry);
            i--;
        }
    }
}

/**
 * IP-Paket versenden und dabei das Routing umgehen und das angegebene Device
 * benutzen.
 */
static bool ip_send_packet_direct(struct device* device, uint32_t dest_ip,
    struct ip_header* header, void* data, size_t size)
{
    uint64_t mac;
    size_t packet_size;
    struct eth_packet_header* eth;

    // Source-IP im Header eintagen und Pruefsumme berechnen
    header->source_ip = device->ip;
    ip_update_checksum(header);


    packet_size = size + sizeof(struct eth_packet_header) +
        sizeof(struct ip_header);
    char packet[packet_size];

    // MAC-Adresse ausfindig machen, an die das Paket gesendet wird
    if (dest_ip == 0xffffffff) {
        mac = 0xffffffffffffULL;
    } else {
        if (!(mac = arp_resolve(device, dest_ip))) {
            return false;
        }
    }

    // Paket zusammenkopieren in unseren Lokalen Puffer
    memcpy(packet + sizeof(struct eth_packet_header), header,
        sizeof(struct ip_header));
    memcpy(packet + sizeof(struct eth_packet_header) +
        sizeof(struct ip_header), data, size);

    // Ethernet-Header befuellen
    eth = (void *) packet;
    eth->src = device->dev.mac;
    eth->dest = mac;
    eth->type = ETH_TYPE_IP;

    // Paket versenden
    ethernet_send_packet(device, packet, packet_size);
    return true;
}


bool ip_send_packet_route(struct ip_header* header, void* data, uint32_t size,
    struct routing_entry* route)
{
    uint32_t dest_ip;

    // Passende Route suchen
    if (route == NULL) {
        route = get_routing_entry(header->dest_ip);
        if (!(route = get_routing_entry(header->dest_ip))) {
            return false;
        }
    }

    // Wenn ein Router eingetragen ist, darueber versenden
    dest_ip = route->gateway ? route->gateway : header->dest_ip;

    return ip_send_packet_direct(route->device, dest_ip, header, data, size);
}

bool ip_send_packet(struct ip_header* header, void* data, uint32_t size)
{
    return ip_send_packet_route(header, data, size, NULL);
}

/**
 * IP-Header befuellen
 *
 * @param header    Pointer auf den Header
 * @param dest_ip   Zieladresse
 * @param proto     Benutztes Protokoll
 * @param data_size Groesse der Daten
 */
static void ip_fill_header(struct ip_header* header, uint32_t dest_ip,
    uint8_t proto, size_t data_size)
{
    header->version         = 4;
    header->ihl             = 5;
    header->type_of_service = 0;
    header->total_length    = big_endian_word(sizeof(*header) + data_size);
    header->id              = big_endian_word(1);
    header->fragment_offset = big_endian_word(0);
    header->ttl             = 255;
    header->protocol        = proto;
    header->checksum        = 0;
    header->source_ip       = 0;
    header->dest_ip         = dest_ip;
}

bool ip_send_route(uint32_t dest_ip, uint8_t proto, void* data,
    size_t data_size, struct routing_entry* route)
{
    struct ip_header ip_header;
    ip_fill_header(&ip_header, dest_ip, proto, data_size);
    return ip_send_packet_route(&ip_header, data, data_size, route);
}

bool ip_send(uint32_t dest_ip, uint8_t proto, void* data, size_t data_size)
{
    return ip_send_route(dest_ip, proto, data, data_size, NULL);
}

/**
 * IP-Paket versenden. Dabei wird das Routing umgangen und direkt die angegebene
 * Netzwerkkarte benutzt.
 */
bool ip_send_direct(struct device* device, uint32_t dest_ip, uint8_t proto,
    void* data, size_t data_size)
{
    struct ip_header ip_header;
    ip_fill_header(&ip_header, dest_ip, proto, data_size);
    return ip_send_packet_direct(device, dest_ip, &ip_header, data, data_size);
}

void ip_receive(struct device *device, void *data, size_t data_size)
{
    struct ip_header* header = data;

    if (data_size < sizeof(struct ip_header)) {
        DEBUG_MSG1("Paket mit zu kleiner Groesse: %d", data_size);
        return;
    }

    uint16_t total_length = big_endian_word(header->total_length);
    uint8_t ihl = header->ihl;

    // Wenn data_size gr��er als total_length ist, darf das Paket nicht
    // verworfen werden, da Ethernetpakete eine Mindestgr��e haben und
    // daher gr��er sein k�nnen als es von IP aus eigentlich n�tig w�re
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

    // TODO Pr�fen, ob das Paket �berhaupt f�r uns bestimmt ist
    // Ansonsten sollte man es theoretisch weiterleiten
    
    // Wenn entweder das Flag IP_FLAG_MORE_FRAGMENTS gesetzt oder
    // die Fragmentnummer gr��er als Null ist, muss zusammengesetzt
    // werden.
    if (big_endian_word(header->fragment_offset) & ~IP_FLAG_DONT_FRAGMENT) {
        DEBUG_MSG("Fragmentiertes Paket");
        return;
    }
    
    DEBUG_MSG("rpc_ip_receive");
    DEBUG_MSG1("data_size: %08x", data_size);
    DEBUG_MSG1("total_len: %08x", total_length);
    DEBUG_MSG1("ihl:     : %08x", ihl);
    DEBUG_MSG1("Source-IP: %08x", header->source_ip);
    DEBUG_MSG1("Dest-IP:   %08x", header->dest_ip);

    void* packet = data + (ihl * 4);
    uint32_t packet_size = total_length - (ihl * 4);
    
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
            udp_receive(header->source_ip, packet, packet_size);
            break;

        default:
            DEBUG_MSG1("Unbekanntes Protokoll %d", header->protocol);
            break;
    }
}
