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

#ifndef _NETINET_IN_H_
#define _NETINET_IN_H_

#include <sys/socket.h>
#include <stdint.h>

/// Laenge einer IP-Adresse als String
#define INET_ADDRSTRLEN 16

/// Ungueltige IP-Adresse
#define INADDR_NONE 0x00000000

/// Adresse, die fuer alle lokalen IP-Adressen steht
#define INADDR_ANY 0x00000000

typedef uint32_t in_addr_t;
struct in_addr {
    in_addr_t s_addr;
};

/// Adresse fuer das INET-Protokol (IPv4)
struct sockaddr_in {
    sa_family_t     sin_family;
    struct in_addr  sin_addr;
    uint32_t        sin_port;
};

// arpa/inet.h braucht die Definitionen von oben. Eigentlich muesste das hier
// gar nicht eingebunden werden, aber manche Programme erwarten Definitionen
// von dort hier.
#include <arpa/inet.h>

#endif

