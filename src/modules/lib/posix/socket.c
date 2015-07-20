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
#include <string.h>
#include <errno.h>

#include <network.h>
#include <collections.h>

struct socket {
    uint64_t            id;
    struct tree_item    tinfo;

    sa_family_t                     sa_family;
    io_resource_t                   conn;
    size_t                          local_address_len;
    struct sockaddr*                local_address;
    FILE*                           listen_file;
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

    socket->id = __iores_fileno(&socket->conn);
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
int connect(int sock, const struct sockaddr* address, socklen_t address_len)
{
    struct socket* socket = get_socket(sock);
    struct sockaddr_in* inet_addr = (struct sockaddr_in*) address;
    char* ip_string;
    uint16_t port;
    char* path;
    int ret = 0;
    io_resource_t* conn;

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

    conn = lio_compat_open(path, IO_OPEN_MODE_READ | IO_OPEN_MODE_WRITE);
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

    return ret;
}

/**
 * Bindet einen Socket an einen bestimmten TCP-Port (bzw. allgemein an eine
 * Adresse)
 *
 * @param sock Socket, dem die Adresse zugewiesen werden soll
 * @param address Adresse fuer den Socket (Fuer IP: struct sockaddr_in)
 * @param address_len Groesse der Adressstruktur
 *
 * @return 0 bei Erfolg. Im Fehlerfall -1 und errno wird gesetzt.
 */
int bind(int sock, const struct sockaddr* address, socklen_t address_len)
{
    struct socket* socket = get_socket(sock);

    if (socket == NULL) {
        errno = EBADF;
        return -1;
    }

    if (address->sa_family != socket->sa_family) {
        errno = EAFNOSUPPORT;
        return -1;
    }

    // FIXME Das wird beim Schliessen geleakt
    socket->local_address = malloc(address_len);
    socket->local_address_len = address_len;
    memcpy(socket->local_address, address, address_len);

    return 0;
}

/**
 * Benutzt einen Socket, um auf eingehende Verbindungen zu warten
 *
 * @param sock Socket, der auf Verbindungen warten soll
 * @param backlog Wird unter tyndur ignoriert
 *
 * @return 0 bei Erfolg. Im Fehlerfall -1 und errno wird gesetzt.
 */
int listen(int sock, int backlog)
{
    char* path;
    uint16_t port;
    struct socket* socket = get_socket(sock);
    struct sockaddr_in* inet_addr;

    if (socket == NULL) {
        errno = EBADF;
        return -1;
    }

    inet_addr = (struct sockaddr_in*) socket->local_address;
    port = big_endian_word(inet_addr->sin_port);
    if (asprintf(&path, "tcpip:/tcp-listen/:%d", port) < 1) {
        errno = ENOMEM;
        return -1;
    }

    socket->listen_file = fopen(path, "w+");
    free(path);
    if (socket->listen_file == NULL) {
        errno = EACCES;
        return -1;
    }

    return 0;
}

/**
 * Nimmt eine eingehende Verbindung an
 *
 * @param sock Socket, der auf eingehende Verbindungen wartet
 * @param address Pointer auf die Struktur, in der die Adresse der Gegenstelle
 * gespeichert wird.
 * @param address_len Pointer auf die Laaenge des Puffers fuer die Adresse der
 * Gegenstelle. Wenn die gespeicherte Adresse kuerzer ist, wird *address_len
 * auf die tatsaechliche Groesse gesetzt.
 *
 * @return Die (nicht-negative) Nummer des Dateideskriptors der neuen
 * Verbindung bei Erfolg. Im Fehlerfall -1 und errno wird gesetzt.
 */
int accept(int sock, struct sockaddr* address, socklen_t* address_len)
{
    char buf[64];
    int clientsock;
    struct socket* clientsocket;
    io_resource_t* conn;

    struct socket* socket = get_socket(sock);
    size_t i;

    if (socket == NULL) {
        errno = EBADF;
        return -1;
    }

    if (socket->listen_file == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (fgets(buf, sizeof(buf), socket->listen_file) == NULL) {
        errno = EAGAIN;
        return -1;
    }

    i = strlen(buf);
    if ((i > 0) && (buf[i - 1] == '\n')) {
        buf[i - 1] = '\0';
    }

    clientsocket = calloc(1, sizeof(*clientsocket));
    clientsocket->sa_family = AF_INET;
    clientsock = create_socket(socket);

    conn = lio_compat_open(buf, IO_OPEN_MODE_READ | IO_OPEN_MODE_WRITE);
    if (conn == NULL) {
        free(socket);
        errno = EAGAIN;
        return -1;
    }
    setvbuf((FILE*)conn, NULL, _IONBF, 0);

    // FIXME Boeser Hack
    memcpy(&socket->conn, conn, sizeof(*conn));
    free(conn);

    // TODO Adresse aus buf rausfummeln
    *address_len = 0;

    return clientsock;
}

/**
 * Gibt die lokale Adresse des Sockets zurueck
 */
int getsockname(int sock, struct sockaddr* address, socklen_t* address_len)
{
    struct socket* socket = get_socket(sock);
    size_t len;

    if (socket == NULL) {
        errno = EBADF;
        return -1;
    }

    if (socket->local_address == NULL) {
        errno = EINVAL;
        return -1;
    }

    len = *address_len < socket->local_address_len
         ? *address_len : socket->local_address_len;
    memcpy(address, socket->local_address, len);

    *address_len = len;

    return 0;
}

/**
 * Liest eine Anzahl Bytes aus einem Socket
 *
 * TODO Mit den Flags was sinnvolles machen
 */
ssize_t recv(int socket, void *buffer, size_t length, int flags)
{
    return read(socket, buffer, length);
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

/**
 * Liest eine Anzahl Bytes aus einem Socket
 *
 * Fuer TCP werden die zusaetzlichen Parameter ignoriert
 */
ssize_t recvfrom(int socket, void *buffer, size_t length, int flags,
    const struct sockaddr* from, socklen_t* from_len)
{
    return recv(socket, buffer, length, flags);
}

/**
 * Sendet eine Anzahl Bytes ueber einen Socket
 *
 * Fuer TCP werden die zusaetzlichen Parameter ignoriert
 */
ssize_t sendto(int socket, const void *buffer, size_t length, int flags,
    const struct sockaddr* to, socklen_t to_len)
{
    return send(socket, buffer, length, flags);
}
