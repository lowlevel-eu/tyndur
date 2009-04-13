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

#include <netinet/in.h>
#include <network.h>
#include <netdb.h>

#include <stdio.h>

unsigned long int htonl(unsigned long int hostlong)
{
    return big_endian_dword(hostlong);
}

unsigned short int htons(unsigned short int hostshort)
{
    return big_endian_word(hostshort);
}

unsigned long int ntohl(unsigned long int netlong)
{
    return big_endian_dword(netlong);
}

unsigned short int ntohs(unsigned short int netshort)
{
    return big_endian_dword(netshort);
}

struct hostent* gethostbyname(const char* name)
{
    static struct hostent hostent;
    static char* aliases[] = { NULL };
    static uint32_t ip;
    static uint32_t* h_addr_list[] = { &ip, NULL };

    // Erstmal schauen, ob es eine IP-Adresse ist
    ip = string_to_ip(name);

    // Wenn nicht, muessen wir eine DNS-Anfrage machen
    if (ip == 0) {
        char* path;
        FILE* f;
        char ip_str[16];

        asprintf(&path, "tcpip:/dns/%s", name);
        f = fopen(path, "r");
        if (!f) {
            return NULL;
        }
        fread(ip_str, 1, 16, f);
        fclose(f);

        ip = string_to_ip(ip_str);
    }

    // hostent aktualisieren
    hostent.h_name = (char*) name;
    hostent.h_aliases = aliases;
    hostent.h_addrtype = AF_INET;
    hostent.h_length = 4;
    hostent.h_addr_list = (char**) h_addr_list;

    return &hostent;
}
