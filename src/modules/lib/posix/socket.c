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

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <lostio.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <network.h>
#include <collections.h>

struct socket {
    uint64_t            id;
    struct tree_item    tinfo;

    sa_family_t         sa_family;
    struct lostio_internal_file conn;
};

static tree_t* sockets = NULL;

/**
 * Ruft die Socketstruktur anhand des Filedeskriptors ab
 */
static struct socket* get_socket(int sock)
{
    if (sockets == NULL) {
        return NULL;
    }

    return tree_search(sockets, sock);
}

/**
 * Ruft die Socketstruktur anhand des Filedeskriptors ab
 */
static int create_socket(struct socket* socket)
{
    if (sockets == NULL) {
        sockets = tree_create(struct socket, tinfo, id);
    }

    socket->id = fileno(&socket->conn.res);
    tree_insert(sockets, socket);
    return socket->id;
}

/**
 * Erstellt einen Socket
 *
 * @param domain Protokollfamilie (AF_*, z.B. AF_INET fuer IPv4)
 * @param type Protokolltyp (SOCK_*, z.B. SOCK_STREAM fuer TCP)
 * @param protocol Wird ignoriert; tyndur benutzt immer das Default-Protokoll,
 * das zu domain und type passt
 *
 * @return Socketnummer. Im Fehlerfall -1 und errno wird gesetzt.
 *
 * FIXME protocol wird ignoriert
 */
int socket(int domain, int type, int protocol)
{
    int sock;
    struct socket* socket;

    if (domain != AF_INET) {
        errno = EAFNOSUPPORT;
        return -1;
    }

    if (type != SOCK_STREAM) {
        errno = EPROTOTYPE;
        return -1;
    }

    socket = calloc(1, sizeof(*socket));
    socket->sa_family = domain;
    memset(&socket->conn, 0, sizeof(socket->conn));

    sock = create_socket(socket);

    return sock;
}

/**
 * Verbindet einen Socket (als Client) mit einer Gegenstelle
 *
 * @param socket Socket, der verbunden werden soll
 * @param address Adresse der Gegenstelle (z.B. IP-Adresse/TCP-Port)
 * @param address_len  Laenge der Adresse in Bytes
 *
 * @return 0 bei Erfolg. Im Fehlerfall -1 und errno wird gesetzt.
 */
int connect(int sock, struct sockaddr* address, socklen_t address_len)
{
    struct socket* socket = get_socket(sock);
    struct sockaddr_in* inet_addr = (struct sockaddr_in*) address;
    char* ip_string;
    uint16_t port;
    char* path;
    int ret = 0;
    struct lostio_internal_file* conn;

    if (socket == NULL) {
        errno = EBADF;
        return -1;
    }

    if (address->sa_family != socket->sa_family) {
        errno = EAFNOSUPPORT;
        return -1;
    }

    ip_string = ip_to_string(inet_addr->sin_addr.s_addr);
    port = big_endian_word(inet_addr->sin_port);
    if (asprintf(&path, "tcpip:/%s:%d", ip_string, port) < 0)
    {
        errno = ENOMEM;
        ret = -1;
        goto out_ip_string;
    }

    conn = (struct lostio_internal_file*) fopen(path, "r+");
    if (conn == NULL) {
        errno = ETIMEDOUT;
        ret = -1;
        goto out_path;
    }
    setvbuf((FILE*)conn, NULL, _IONBF, 0);

    // FIXME Boeser Hack
    memcpy(&socket->conn, conn, sizeof(*conn));
    free(conn);

out_path:
    free(path);
out_ip_string:
    free(ip_string);

    return 0;
}

/**
 * Liest eine Anzahl Bytes aus einem Socket
 *
 * TODO Mit den Flags was sinnvolles machen
 */
ssize_t recv(int socket, const void *buffer, size_t length, int flags)
{
    return read(socket, (void*) buffer, length);
}

/**
 * Sendet eine Anzahl Bytes ueber einen Socket
 *
 * TODO Mit den Flags was sinnvolles machen
 */
ssize_t send(int socket, const void *buffer, size_t length, int flags)
{
    return write(socket, buffer, length);
}
