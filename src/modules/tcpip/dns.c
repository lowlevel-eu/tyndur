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
#include <string.h>
#include <network.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

#include "dns.h"
#include "tcp.h"
#include "main.h"

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

#define DNS_CACHE_SIZE 64

struct dns_cache_entry {
    char*   name;
    struct dns_request_result* result;
};

static list_t* dns_cache = NULL;

/**
 * Holt eine IP-Adresse aus dem DNS-Cache
 *
 * @return Zum gegebenen Domainnamen passende IP-Adresse oder 0, wenn die
 * Domain nicht im Cache ist.
 */
static struct dns_request_result* dns_cache_get(char* domain)
{
    struct dns_cache_entry* entry;
    int i;
    
    DEBUG_MSG1("dns: cache_get: %s\n", domain);

    for (i = 0; (entry = list_get_element_at(dns_cache, i)); i++) {
        if (strcmp(domain, entry->name) == 0) {
            DEBUG_MSG1(" -- %s: Treffer\n", entry->name);
            return entry->result;
        }
        DEBUG_MSG1(" -- %s: Treffer\n", entry->name);
    }
    DEBUG_MSG1(" -- %s: Keine Treffer\n",domain);
    return NULL;
}

/**
 * Gibt die Ressourcen eines dns_request_result wieder frei
 * @param result Das freizugebene Resultat
 */
static void dns_free_result(struct dns_request_result* result)
{
    free(result->ip_address);
    free(result);
}

/**
 * Fuegt dem DNS-Cache einen neuen Eintrag hinzu. Dabei wird unter Umstaenden
 * ein alter Eintrag geloescht, um den Cache auf einer ertraeglichen Groesse zu
 * halten.
 */
static void dns_cache_add(char* domain, struct dns_request_result* result)
{
    struct dns_cache_entry* entry = malloc(sizeof(*entry));
    size_t domain_len = strlen(domain);

    entry->result = result;
    entry->name = malloc(domain_len + 1);
    strncpy(entry->name, domain, domain_len);
    entry->name[domain_len] = '\0';

    DEBUG_MSG1("dns: cache_add: %s\n", entry->name);

    list_push(dns_cache, entry);

    if (list_size(dns_cache) > DNS_CACHE_SIZE) {
        dns_free_result(list_get_element_at(dns_cache,DNS_CACHE_SIZE));
        list_remove(dns_cache, DNS_CACHE_SIZE);
    }
}

/**
 * Loest einen Domainnamen in alle zugehoerigen IP-Adressen auf.
 *
 * @param domain Der aufzuloesende Domainname als nullterminierter String,
 * wobei die Labels durch Punkte getrennt sind
 */
struct dns_request_result* dns_request(char* domain)
{
    struct dns_request_result* dns_result;
    uint32_t result;

    // Cache anlegen, falls noch keiner da ist
    if (dns_cache == NULL) {
        DEBUG_MSG("DNS: Erzeuge Cache\n");
        dns_cache = list_create();
    }

    // Wenn es im Cache ist, daraus nehmen
    dns_result = dns_cache_get(domain);
    if (dns_result != NULL) {
        return dns_result;
    }

    dns_result = malloc(sizeof(struct dns_request_result));

    // Ansonsten brauchen wir eine Verbindung zum DNS-Server
    uint32_t dns_ip = options.nameserver;

    DEBUG_MSG("DNS: Oeffne Verbindung\n");
    struct tcp_connection* conn = tcp_open_connection(dns_ip, DNS_PORT);
    if (conn == NULL) {
        DEBUG_MSG("DNS: Konnte Verbindung nicht aufbauen.\n");
        return NULL;
    }

    // Puffer fuer die Nachricht reservieren:
    //   2 Bytes:   TCP-Praefix (enthaelt die Laenge der Anfrage)
    //   x Bytes:   DNS-Header
    // y+2 Bytes:   Domainname (zwei Zeichen zusaetzlich fuer die Laenge des
    //              ersten Labels und ein abschliessendes Nullbyte)
    //   4 Bytes:   QTYPE und QCLASS
    size_t domain_len = strlen(domain);
    size_t message_size = 2 + sizeof(struct dns_header) + domain_len + 6;

    char data[message_size];

    struct dns_header* header = (struct dns_header*) &data[2];
    char* qname     = &data[2 + sizeof(struct dns_header)];
    uint16_t* qtype     = (uint16_t*) &qname[domain_len + 2];
    uint16_t* qclass    = (uint16_t*) &qname[domain_len + 4];

    // 2 Bytes Praefix f�r TCP-Transport (Laenge)
    *((uint16_t*) data) =
        big_endian_word(sizeof(struct dns_header) + domain_len + 6);
    
    // DNS-Header
    header->id = 0;
    header->flags = DNS_RD;
    header->qd_count = big_endian_word(1);
    header->an_count = 0;
    header->ns_count = 0;
    header->ar_count = 0;
    
    // Question Section
    // Die Question Section besteht aus einzelnen Labels, d.h. Bestandteilen
    // einer Domain (www.xyz.de hat die drei Labels www, xyz und de).
    // Vor jedem Label gibt ein Byte die Laenge dieses Labels an. Daher von
    // hinten durchgehen und mitzaehlen und bei jedem Punkt den aktuellen
    // Zaehlerstand setzen.
    size_t label_len = 0;
    int i;

    for (i = domain_len; i; i--) {
        if (domain[i - 1] != '.') {
            qname[i] = domain[i - 1];
            label_len++;
        } else {
            qname[i] = label_len;
            label_len = 0;
        }

        // Ein gueltiges Label darf maximal 63 Zeichen enthalten
        if (label_len > 63) {
            tcp_close_connection(conn);
            return 0;
        }
    }
    qname[0] = label_len;

    // Das Ende wird durch ein Label der Laenge 0 markiert
    qname[domain_len + 1] = '\0';

    // QTYPE = A und QCLASS = IN
    *qtype = QTYPE_A;
    *qclass = QCLASS_IN;

    // Senden 
    struct tcp_out_buffer out_buffer = {
        .size = message_size,
        .data = data
    };
    list_insert(conn->out_buffer, list_size(conn->out_buffer), &out_buffer);
    tcp_send_ack(conn);

    // Auf Antwort warten
    DEBUG_MSG("DNS: Warte auf Antwort\n");
    struct tcp_in_buffer* in_buffer;

    // Zaehler fuer den Index von dns_result
    int dns_result_index = 0;
    while(1) {
        if ((in_buffer = list_pop(conn->to_lostio))) {
            void* reply = in_buffer->data;

            // 2 Bytes TCP-Pr�fix
            reply += 2;

            // Header ignorieren
            int qd_count = 
                big_endian_word(((struct dns_header*) reply)->qd_count);
            int an_count = 
                big_endian_word(((struct dns_header*) reply)->an_count);
            reply += sizeof(struct dns_header);

            // Speicher fuer die IPs holen
            dns_result->ip_address = malloc(sizeof(uint32_t) * an_count);

            // Question Section ignorieren
            for (i = 0; i < qd_count; i++) {                
                // QNAMEs
                int label_len;
                while ((label_len = *((uint8_t*) reply))) {
                    reply += label_len + 1;
                }

                // Null-Label, QTYPE und QCLASS
                reply += 5;
            }

            // Hier kommt die Antwort - alles ignorieren, was den falschen
            // Typ hat, und beim ersten Treffer die IP zur�ckgeben.
            for (i = 0; i < an_count; i++) 
            {
                // QNAMEs - interessieren nicht
                int label_len;
                while ((label_len = *((uint8_t*) reply))) {
                    
                    if (label_len & 0xC0) {
                        // Pointer
                        reply += 2;
                        break;
                    } else {
                        int j;
                        for (j = 0; j < label_len; j++) {
                            printf(" %02hhx", *((uint8_t*) reply + j));
                        }
                        reply += label_len + 1;
                    }
                }

                // QTYPE - Hostadressen (A = 0x1 big-endian)  interessieren
                size_t answer_len = 
                    10 + big_endian_word(*((uint16_t*) (reply + 8)));

                if (*((uint16_t*) reply) == 0x100) {
                    result = *((uint32_t*) (reply + 10));

                    dns_result->ip_address[dns_result_index] = result;
                    dns_result_index++;

                    reply += answer_len;
                } else {
                    result = 0;
                    reply += answer_len;
                }

            }

            dns_result->ip_count = dns_result_index;
            break;
        }
        wait_for_rpc();
    }

    tcp_close_connection(conn);

    dns_cache_add(domain,dns_result);

    return dns_result;
}
