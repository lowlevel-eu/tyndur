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

#ifndef _ARPA_INET_H_
#define _ARPA_INET_H_

#include <netinet/in.h>


#ifdef __cplusplus
extern "C" {
#endif

unsigned long int htonl(unsigned long int hostlong);
unsigned short int htons(unsigned short int hostshort);
unsigned long int ntohl(unsigned long int netlong);
unsigned short int ntohs(unsigned short int netshort);

/**
 * Wandelt einen String, der eine IP-Adresse der Form a.b.c.d enthaelt in einen
 * 32-Bit-Wert um.
 *
 * @return 0 im Fehlerfall; ungleich 0 bei Erfolg
 */
int inet_aton(const char* ip_string, struct in_addr* ip);

/**
 * Tut im Grunde das Gleiche wie inet_aton.
 *
 * @return INADDR_NONE (0) im Fehlerfall; Adresswert bei Erfolg
 */
in_addr_t inet_addr(const char* ip_string);

/**
 * Wandelt eine 32-Bit-Adresse in einen String um. Der String ist in einem
 * statischen Puffer und wird beim naechsten Aufruf ueberschrieben.
 */
char* inet_ntoa(struct in_addr ip);

/**
 * Wandelt einen String, der eine Adresse (IPv4 or IPv6) enthaelt in ihre
 * binaere Darstellung um.
 *
 * @param family Adressfamilie, fuer die umgewandelt werden soll (z.B. AF_INET)
 * @param src Adresse als String
 * @param dst Ziel fuer die binaere Darstellung (z.B. struct in_addr*)
 *
 * @return 1 bei Erfolg, 0 wenn die Adresse ungueltig ist. Bei sonstigen
 * Fehlern -1 und errno wird gesetzt.
 */
int inet_pton(int family, const char* src, void* dst);

/**
 * Wandelt eine Adresse (IPv4 oder IPv6) in einen String um. Der Ausgabepuffer
 * wird vom Aufrufer uebergeben.
 *
 * @param family Adressfamilie, zu der src gehoert (z.B. AF_INET)
 * @param src Adressstruktur, die umgewandelt werden soll (z.B. struct
 * in_addr*)
 * @param dst Ausgabepuffer
 * @param size Laenge des Ausgabepuffers
 *
 * @return Pointer auf den Ausgabepuffer; im Fehlerfall NULL und errno wird
 * gesetzt
 */
const char *inet_ntop(int family, const void* src, char* dst, socklen_t size);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif

