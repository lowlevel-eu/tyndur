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
#ifndef _SYS_SOCKET_H_
#define _SYS_SOCKET_H_

#include <stdint.h>
#include <sys/types.h>

typedef int socklen_t;
typedef unsigned int sa_family_t;

#define PF_INET AF_INET

/**
 *  Beschreibt die Adresse einer Gegenstelle fuer einen Socket.
 *
 *  Diese struct dient als "Vaterklasse" fuer weitere structs, die fuer die
 *  einzelnen Adressfamilien definiert sind.
 */
struct sockaddr {
    /// Adressfamilie (AF_*)
    sa_family_t     sa_family;

    /// Adressfamilienspezifische Daten
    char            sa_data[];
};

struct sockaddr_storage {
    sa_family_t     ss_family;
};

/// Protokolltypen
enum {
    /// Verbindungsoriertierter Socket (z.B. TCP)
    SOCK_STREAM,

    /// Datagramm-Socket (z.B. UDP)
    SOCK_DGRAM,

    /// Rohdaten (direkt IP-Pakete)
    SOCK_RAW,
};

/// Adressfamilien
enum {
    /// IPv4
    AF_INET,
};

/// Protokolle fuer IP
enum {
    IPPROTO_TCP,
    IPPROTO_UDP,
    IPPROTO_ICMP,
    IPPROTO_IP,
    IPPROTO_IPV6,
    IPPROTO_RAW,
};


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Erstellt einen Socket
 *
 * @param domain Protokollfamilie (AF_*, z.B. AF_INET fuer IPv4)
 * @param type Protokolltyp (SOCK_*, z.B. SOCK_STREAM fuer TCP)
 * @param protocol Wird ignoriert; tyndur benutzt immer das Default-Protokoll,
 * das zu domain und type passt
 *
 * @return Socketnummer. Im Fehlerfall -1 und errno wird gesetzt.
 */
int socket(int domain, int type, int protocol);

/**
 * Verbindet einen Socket (als Client) mit einer Gegenstelle
 *
 * @param socket Socket, der verbunden werden soll
 * @param address Adresse der Gegenstelle (z.B. IP-Adresse/TCP-Port)
 * @param address_len  Laenge der Adresse in Bytes
 *
 * @return 0 bei Erfolg. Im Fehlerfall -1 und errno wird gesetzt.
 */
int connect(int socket, const struct sockaddr* address, socklen_t address_len);

/**
 * Gibt die lokale Adresse des Sockets zurueck
 */
int getsockname(int sock, struct sockaddr* address, socklen_t* address_len);

/**
 * Liest eine Anzahl Bytes aus einem Socket
 */
ssize_t recv(int socket, void *buffer, size_t length, int flags);

/**
 * Sendet eine Nachricht ueber einen Socket
 */
ssize_t send(int socket, const void *buffer, size_t length, int flags);

/**
 * Liest eine Anzahl Bytes aus einem Socket
 *
 * Fuer TCP werden die zusaetzlichen Parameter ignoriert
 */
ssize_t recvfrom(int socket, void *buffer, size_t length, int flags,
    const struct sockaddr* from, socklen_t* from_len);

/**
 * Sendet eine Anzahl Bytes ueber einen Socket
 *
 * Fuer TCP werden die zusaetzlichen Parameter ignoriert
 */
ssize_t sendto(int socket, const void *buffer, size_t length, int flags,
    const struct sockaddr* to, socklen_t to_len);

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
int bind(int sock, const struct sockaddr* address, socklen_t address_len);

/**
 * Benutzt einen Socket, um auf eingehende Verbindungen zu warten
 *
 * @param sock Socket, der auf Verbindungen warten soll
 * @param backlog Wird unter tyndur ignoriert
 *
 * @return 0 bei Erfolg. Im Fehlerfall -1 und errno wird gesetzt.
 */
int listen(int sock, int backlog);

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
int accept(int sock, struct sockaddr* address, socklen_t* address_len);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif
