/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
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
#include <types.h>
#include <lostio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>
#include <rpc.h>
#include <network.h>
#include <errno.h>

#include "ip.h"
#include "tcp.h"
#include "lostio_if.h"

extern list_t* tcp_connections;
extern list_t* tcp_servers;

static void do_nothing(void) {}

/**
 * Legt Knoten im Verzeichnisbaum an, die noch nicht existieren, wenn auf sie
 * zugegriffen werden soll
 */
static bool lio_server_dir_not_found(char** path, uint8_t flags, pid_t pid,
    FILE* ps)
{
    return vfstree_create_node(*path, LOSTIO_TYPES_SERVER, 0, NULL, 0);
}

/**
 * Das Oeffnen einer Datei in tcpip:/tcp-listen registriert einen TCP-Server.
 * Wenn ein Client verbindet, wird in der geoeffneten Datei der Dateiname zum
 * Oeffnen der Verbindung ausgegeben
 */
static bool lio_server_pre_open(char** path, uint8_t flags, pid_t pid,
    FILE* ps)
{
    char* s;
    char* port;
    int portnr;
    int i;
    struct tcp_server* server;
    vfstree_node_t* node;

    // Knoten raussuchen
    node = vfstree_get_node_by_path(*path);
    if (node == NULL) {
        return false;
    }

    // Pfad auseinandernehmen
    if (strncmp(*path, "/tcp-listen/", strlen("/tcp-listen/"))) {
        return false;
    }

    s = *path + strlen("/tcp-listen/");
    port = strchr(s, ':');
    if (port == NULL) {
        return false;
    }

    port++;
    portnr = atoi(port);
    if (port == 0) {
        return false;
    }

    // Pruefen, ob der Port nicht schon belegt ist
    p();
    for (i = 0; (server = list_get_element_at(tcp_servers, i)); i++) {
        if (server->port == portnr) {
            v();
            return false;
        }
    }

    // Neuen Server anlegen und der Ressource zuordnen
    server = calloc(1, sizeof(*server));
    server->port = portnr;
    server->requests = list_create();
    node->data = server;

    list_push(tcp_servers, server);
    v();

    return true;
}

/**
 * Liest eine Verbindung, die geoeffnet werden kann, aus
 */
size_t lio_server_read(lostio_filehandle_t* fh, void* buf,
    size_t blocksize, size_t count)
{
    struct tcp_server* server = fh->node->data;
    char* s;
    size_t len;

    s = list_pop(server->requests);
    if (s == NULL) {
        return 0;
    }

    len = strlen(s);
    if (blocksize * count < len) {
        len = blocksize * count;
    }


    strncpy(buf, s, len);
    free(s);

    return len;
}

/**
 * Beendet einen Server
 */
int lio_server_close(lostio_filehandle_t* fh)
{
    struct tcp_server* server = fh->node->data;
    struct tcp_server* cur;
    char* s;
    int i;

    // Aus der Liste der Server entfernen
    p();
    for (i = 0; (cur = list_get_element_at(tcp_servers, i)); i++) {
        if (server == cur) {
            list_remove(tcp_servers, i);
            break;
        }
    }
    v();

    // VFS-Knoten entfernen
    fh->node->data = NULL;
    while ((s = list_pop(server->requests))) {
        free(s);
    }
    list_destroy(server->requests);
    free(server);
    vfstree_delete_child(fh->node->parent, fh->node->name);

    return 0;
}

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
    struct tcp_header* header, struct tcp_connection** res_conn)
{
    struct tcp_server* server;
    struct tcp_connection* conn;
    struct routing_entry* route;
    uint64_t timeout;
    int i;

    uint16_t remote_port = big_endian_word(header->source_port);
    uint16_t local_port = big_endian_word(header->dest_port);

    // Server an header->dest_port suchen
    server = NULL;
    for (i = 0; (server = list_get_element_at(tcp_servers, i)); i++) {
        if (server->port == local_port) {
            break;
        }
    }

    if (server == NULL) {
        return -ENOENT;
    }

    // Duplikate suchen
    // Wenn wir sowieso schon darauf warten, dass jemand die Verbindung
    // annimmt, hilft uns ein zweites SYN-Paket auch nicht weiter.
    p();
    conn = NULL;
    for (i = 0; (conn = list_get_element_at(tcp_connections, i)); i++) {
        if ((conn->source_port  == local_port) &&
            (conn->dest_port    == remote_port) &&
            (conn->dest_ip      == remote_ip))
        {
            v();
            return -EEXIST;
        }
    }

    // Verbindung erstellen
    conn = calloc(1, sizeof(struct tcp_connection));
    route = get_routing_entry(remote_ip);

    if (route == NULL) {
        v();
        return -EIO;
    }

    conn->dest_ip = remote_ip;
    conn->dest_port = remote_port;
    conn->source_ip = route->device->ip;
    conn->source_port = local_port;
    conn->window = 0x1000;
    conn->ack = big_endian_dword(header->seq) + 1;
    conn->out_buffer = list_create();
    conn->in_buffer = list_create();
    conn->to_lostio = list_create();
    conn->status = TCPS_WAIT_FOR_ACCEPT;

    list_push(tcp_connections, conn);
    v();

    // Neuen VFS-Knoten anlegen
    char* ip_str = ip_to_string(remote_ip);
    char* buf;
    asprintf(&buf, "/tcp-conn/local:%d_%s:%d",
        local_port, ip_str, remote_port);
    vfstree_create_node(buf, LOSTIO_TYPES_SERVER_CONN, 0, conn,
        LOSTIO_FLAG_NOAUTOEOF);
    free(buf);

    // Den Server informieren
    asprintf(&buf, "tcpip:/tcp-conn/local:%d_%s:%d\n",
        local_port, ip_str, remote_port);
    free(ip_str);
    p();
    list_push(server->requests, buf);
    v();

    // Warten, bis der Server die Verbindung annimmt
    timeout = get_tick_count() + 9 * 1000000;
    timer_register(do_nothing, 10 * 1000000);
    while ((conn->status == TCPS_WAIT_FOR_ACCEPT)
        && (get_tick_count() < timeout))
    {
        wait_for_rpc();
    }

    if (conn->status == TCPS_WAIT_FOR_ACCEPT) {
        // TODO Verbindung abbrechen und wieder freigeben
        return -ETIMEDOUT;
    }

    *res_conn = conn;
    return 0;
}

/**
 * Oeffnet eine eingehende Verbindung
 */
void lio_server_conn_post_open(lostio_filehandle_t* fh)
{
    struct tcp_connection* conn = fh->node->data;

    tcp_accept_connection(conn);
    fh->node->size++;
}

/**
 * Schliesst eine eingehende Verbindung und entfernt sie aus dem
 * Verzeichnisbaum.
 */
int lio_server_conn_close(lostio_filehandle_t* fh)
{
    int ret = lostio_tcp_close(fh);

    // VFS-Knoten entfernen
    vfstree_delete_child(fh->node->parent, fh->node->name);

    return ret;
}

/// Typ fuer einen TCP-Server
static typehandle_t lio_type_server = {
    .id          = LOSTIO_TYPES_SERVER,
    .pre_open    = &lio_server_pre_open,
    .read        = &lio_server_read,
    .close       = &lio_server_close,
};

/// Typ fuer eine Verbindung fuer TCP-Server
static typehandle_t lio_type_server_conn = {
    .id          = LOSTIO_TYPES_SERVER_CONN,
    .post_open   = &lio_server_conn_post_open,
    .read        = &lostio_tcp_read,
    .write       = &lostio_tcp_write,
    .close       = &lio_server_conn_close,
};

/**
 * Registriert die LostIO-Typehandles, die fuer TCP-Server benoetigt werden
 */
void tcp_server_register_lostio(void)
{
    // Typ fuer das Verzeichnis tcp-listen
    lostio_type_directory_use_as(LOSTIO_TYPES_SERVER_DIR);
    get_typehandle(LOSTIO_TYPES_SERVER_DIR)->not_found =
        &lio_server_dir_not_found;

    // Typen fuer einen TCP-Server und seine Verbindungen
    lostio_register_typehandle(&lio_type_server);
    lostio_register_typehandle(&lio_type_server_conn);

    // Knoten erstellen
    vfstree_create_node("/tcp-listen", LOSTIO_TYPES_SERVER_DIR, 0, NULL,
        LOSTIO_FLAG_BROWSABLE);
    vfstree_create_node("/tcp-conn", LOSTIO_TYPES_DIRECTORY, 0, NULL,
        LOSTIO_FLAG_BROWSABLE);
}
