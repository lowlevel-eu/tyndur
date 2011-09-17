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

#include "udp.h"
#include "ip.h"
#include "dns.h"
#include "lostio_if.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <syscall.h>
#include <stdint.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

struct udp_socket {
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t local_ip;
    uint32_t remote_ip;

    list_t* incoming_packets;
    struct routing_entry* route;
};

struct udp_packet {
    size_t length;
    char data[];
};

static list_t* udp_sockets;

/**
 * Parst einen gegebenen String, der eine Verbindung im Format
 * <Lokaler Port>:<Remote-Hostname>:<Remote-Port> beschreibt.
 *
 * Rückgabe:
 *       0: Erfolg
 * -EINVAL: Falsches Format
 * -ENOENT: Der DNS-Name konnte nicht aufgelöst werden
 */
static int parse_connection(const char* filename, int* sport,
    uint32_t* daddr, int* dport)
{
    int ret, tmp_sport, tmp_dport;
    uint32_t tmp_daddr;
    char hostname[64];

    /* Verzeichnisname abschneiden */
    if (strncmp(filename, "/udp/", strlen("/udp/"))) {
        return -EINVAL;
    }

    filename = filename + strlen("/udp/");

    /* Wenn der Aufrufer das Ergebnis nicht braucht, lokale Variablen nehmen */
    if (sport == NULL || daddr == NULL || dport == NULL) {
        sport = &tmp_sport;
        daddr = &tmp_daddr;
        dport = &tmp_dport;
    }

    /* Dateiname auseinandernehmen */
    ret = sscanf(filename, "%d:%63[^:]:%d", sport, hostname, dport);
    if (ret != 3) {
        return -EINVAL;
    }

    /* Servername auflösen, wenn es keine IP-Adresse war */
    uint32_t ip = string_to_ip(hostname);
    if (!ip) {
        struct dns_request_result* result = dns_request(hostname);
        if (result && (result->ip_count > 0)) {
            ip = result->ip_address[0];
        }
    }

    if (!ip) {
        return -ENOENT;
    }

    *daddr = ip;
    return 0;
}

/**
 * Legt Knoten im Verzeichnisbaum an, die noch nicht existieren, wenn auf sie
 * zugegriffen werden soll
 */
static bool lio_udp_dir_not_found(char** path, uint8_t flags, pid_t pid,
    FILE* ps)
{
    int ret;

    ret = parse_connection(*path, NULL, NULL, NULL);
    if (ret < 0) {
        return false;
    }

    return vfstree_create_node(*path, LOSTIO_TYPES_UDP, 0, NULL,
        LOSTIO_FLAG_NOAUTOEOF);
}

/**
 * Das Oeffnen einer Datei in tcpip:/tcp-listen registriert einen TCP-Server.
 * Wenn ein Client verbindet, wird in der geoeffneten Datei der Dateiname zum
 * Oeffnen der Verbindung ausgegeben
 */
static bool lio_udp_pre_open(char** path, uint8_t flags, pid_t pid,
    FILE* ps)
{
    struct udp_socket* s;
    vfstree_node_t* node;
    int ret, sport, dport;
    uint32_t daddr;

    /* Knoten raussuchen */
    node = vfstree_get_node_by_path(*path);
    if (node == NULL) {
        return false;
    }

    /* Pfad auseinandernehmen */
    ret = parse_connection(*path, &sport, &daddr, &dport);
    if (ret < 0) {
        return false;
    }

    /* UDP-Verbindung oeffnen */
    s = udp_open(sport, daddr, dport);
    if (s == NULL) {
        return false;
    }

    node->data = s;
    return true;
}


/**
 * Öffnet einen UDP-Clientsocket
 *
 * @param sport Lokale Portnummer
 * @param daddr IP-Adresse des Remote-Hosts
 * @param dport Portnummer des Remote-Hosts
 *
 * @return Der geöffnete Socket oder NULL bei Fehlern
 */
struct udp_socket* udp_open(uint16_t sport, uint32_t daddr, uint16_t dport)
{
    struct udp_socket* s;
    struct routing_entry* route;
    int i;

    p();
    route = get_routing_entry(daddr);
    if (route == NULL || route->device == NULL) {
        v();
        return false;
    }

    /* Pruefen, ob der Port nicht schon belegt ist */
    for (i = 0; (s = list_get_element_at(udp_sockets, i)); i++) {
        if (s->local_port == sport) {
            v();
            return false;
        }
    }

    /* Neuen Socket anlegen und der Ressource zuordnen */
    s = calloc(1, sizeof(*s));
    s->local_port = sport;
    s->local_ip = route->device->ip;
    s->remote_port = dport;
    s->remote_ip = daddr;
    s->incoming_packets = list_create();

    list_push(udp_sockets, s);
    v();

    return s;
}

/**
 * Öffnet einen UDP-Clientsocket über eine gegebene Route (ignoriert die
 * Routentabelle)
 *
 * @param route Route, die benutzt werden soll
 * @see udp_open
 */
struct udp_socket* udp_open_route(uint16_t sport, uint32_t daddr,
    uint16_t dport, struct routing_entry* route)
{
    struct udp_socket* s;

    s = udp_open(sport, daddr, dport);
    if (s == NULL) {
        return NULL;
    }

    s->route = route;
    return s;
}

/**
 * LostIO-Wrapper für udp_read
 */
static size_t lio_udp_read(lostio_filehandle_t* fh, void* buf,
    size_t blocksize, size_t count)
{
    struct udp_socket* s = fh->node->data;

    return udp_read(s, buf, blocksize * count);
}

/**
 * Liest ein einzelnes UDP-Paket aus
 */
size_t udp_read(struct udp_socket* s, void* buf, size_t len)
{
    struct udp_packet* p;

    p = list_pop(s->incoming_packets);
    if (p == NULL) {
        return 0;
    }

    len = MIN(len, p->length);
    memcpy(buf, p->data, len);
    free(p);

    return len;
}

/**
 * Empfängt ein eingehendes UDP-Paket
 */
void udp_receive(uint32_t source_ip, void* data, uint32_t data_size)
{
    struct udp_header* hdr = data;
    struct udp_packet* p;
    struct udp_socket* s;
    void* payload;
    size_t payload_size;
    size_t pktlen;
    int i;

    if (data_size < sizeof(*hdr)) {
        return;
    }

    p();
    for (i = 0; (s = list_get_element_at(udp_sockets, i)); i++) {
        if (  (s->remote_ip   == source_ip || s->remote_ip == 0xffffffff)
            && s->remote_port == big_endian_word(hdr->src_port)
            && s->local_port  == big_endian_word(hdr->dest_port))
        {
            goto found;
        }
    }
    goto out;

found:
    pktlen = big_endian_word(hdr->length);
    if (pktlen > data_size) {
        goto out;
    }

    payload = hdr + 1;
    payload_size = pktlen - sizeof(struct udp_header);

    struct udp_pseudo_header phdr = {
        .source_ip  = source_ip,
        .dest_ip    = s->local_ip,
        .proto      = big_endian_word(IP_PROTO_UDP),
        .length     = big_endian_word(data_size),
    };

    if (udp_checksum(hdr, &phdr, payload, payload_size) != 0xffff) {
        printf("\033[1;31mUDP-Checksumme ungueltig\n\033[0;37m\n\n");
        goto out;
    }

    p = malloc(sizeof(*p) + payload_size);
    p->length = payload_size;
    memcpy(p->data, payload, payload_size);

    list_insert(s->incoming_packets, list_size(s->incoming_packets), p);

out:
    v();
}

/**
 * LostIO-Wrapper für udp_write
 */
static size_t lio_udp_write(lostio_filehandle_t* fh, size_t blocksize,
    size_t count, void* buf)
{
    struct udp_socket* s = fh->node->data;

    return udp_write(s, buf, blocksize * count);
}

/**
 * Schreibt ein einzelnes UDP-Paket
 */
size_t udp_write(struct udp_socket* s, void* buf, size_t len)
{
    size_t udp_packet_size = len + sizeof(struct udp_header);
    uint8_t udp_packet[udp_packet_size];

    struct udp_header* hdr = (struct udp_header*) udp_packet;
    struct udp_pseudo_header phdr;

    phdr = (struct udp_pseudo_header) {
        .source_ip  = s->local_ip,
        .dest_ip    = s->remote_ip,
        .proto      = big_endian_word(IP_PROTO_UDP),
        .length     = big_endian_word(udp_packet_size),
    };

    *hdr = (struct udp_header) {
        .src_port   = big_endian_word(s->local_port),
        .dest_port  = big_endian_word(s->remote_port),
        .length     = big_endian_word(udp_packet_size),
        .checksum   = 0,
    };

    memcpy(udp_packet + sizeof(struct udp_header), buf, len);
    udp_update_checksum(hdr, &phdr,
        udp_packet + sizeof(struct udp_header), len);

    ip_send_route(s->remote_ip, IP_PROTO_UDP, udp_packet, udp_packet_size,
        s->route);

    return len;
}

/**
 * LostIO-Wrapper für udp_close
 */
static int lio_udp_close(lostio_filehandle_t* fh)
{
    struct udp_socket* s = fh->node->data;


    udp_close(s);
    fh->node->data = NULL;
    vfstree_delete_child(fh->node->parent, fh->node->name);

    return 0;
}

/**
 * Beendet eine UDP-Verbindung
 */
void udp_close(struct udp_socket *s)
{
    struct udp_socket* cur;
    struct udp_packet* p;
    int i;

    /* Aus der Liste der UDP-Sockets entfernen */
    p();
    for (i = 0; (cur = list_get_element_at(udp_sockets, i)); i++) {
        if (s == cur) {
            list_remove(udp_sockets, i);
            break;
        }
    }
    v();

    /* Übrige Pakete wegwerfen */
    while ((p = list_pop(s->incoming_packets))) {
        free(p);
    }
    list_destroy(s->incoming_packets);
    free(s);
}

/** Typ fuer einen UDP-Socket */
static typehandle_t lio_type_udp = {
    .id          = LOSTIO_TYPES_UDP,
    .pre_open    = &lio_udp_pre_open,
    .read        = &lio_udp_read,
    .write       = &lio_udp_write,
    .close       = &lio_udp_close,
};

/**
 * Registriert die LostIO-Typehandles, die fuer UDP benoetigt werden
 */
void udp_register_lostio(void)
{
    udp_sockets = list_create();

    /* Typ fuer das Verzeichnis udp */
    lostio_type_directory_use_as(LOSTIO_TYPES_UDP_DIR);
    get_typehandle(LOSTIO_TYPES_UDP_DIR)->not_found = &lio_udp_dir_not_found;

    /* Typen fuer einen TCP-Server und seine Verbindungen */
    lostio_register_typehandle(&lio_type_udp);

    /* Knoten erstellen */
    vfstree_create_node("/udp", LOSTIO_TYPES_UDP_DIR, 0, NULL,
        LOSTIO_FLAG_BROWSABLE);
}
