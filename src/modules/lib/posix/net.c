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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int h_errno;

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
    return big_endian_word(netshort);
}

/**
 * Gibt den Fehlerstring fuer einen gegebenen Wert von h_errno zurueck
 */
const char* hstrerror(int error)
{
    static const char* errors[] = {
        [HOST_NOT_FOUND]    = "Unbekannter Rechner",
        [NO_DATA]           = "Der gesuchte Name hat keine IP-Adresse",
        [NO_RECOVERY]       = "Ein DNS-Fehler ist aufgetreten",
        [TRY_AGAIN]         = "Zeitweiliger DNS-Fehler; erneut versuchen",
    };

    if ((error < HOST_NOT_FOUND) || (error > TRY_AGAIN)) {
        return NULL;
    }

    return errors[error];
}

/**
 * Gibt str gefolgt vom String fuer h_errno auf stderr aus. Wenn str NULL oder
 * ein leerer String ist, wird nur der Fehlerstring ausgegeben.
 */
void herror(const char* str)
{
    if (str && *str) {
        fprintf(stderr, "%s: %s\n", str, hstrerror(h_errno));
    } else {
        fprintf(stderr, "%s\n", hstrerror(h_errno));
    }
}



/**
 * Wandelt einen String, der eine IP-Adresse der Form a.b.c.d enthaelt in einen
 * 32-Bit-Wert um.
 *
 * @return 0 im Fehlerfall; ungleich 0 bei Erfolg
 * FIXME 0.0.0.0 ist eine gueltige Adresse
 */
int inet_aton(const char* ip_string, struct in_addr* ip)
{
    ip->s_addr = string_to_ip((char*) ip_string);
    return ip->s_addr;
}

/**
 * Tut im Grunde das Gleiche wie inet_aton.
 *
 * @return INADDR_NONE (0) im Fehlerfall; Adresswert bei Erfolg
 */
in_addr_t inet_addr(const char* ip_string)
{
    return string_to_ip((char*) ip_string);
}

/**
 * Wandelt eine 32-Bit-Adresse in einen String um. Der String ist in einem
 * statischen Puffer und wird beim naechsten Aufruf ueberschrieben.
 */
char* inet_ntoa(struct in_addr ip)
{
    static char buf[16];
    char* res;

    res = ip_to_string(ip.s_addr);
    strncpy(buf, res, 16);
    free(res);

    return buf;
}
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
int inet_pton(int family, const char* src, void* dst)
{
    struct in_addr* addr = dst;

    if (family != AF_INET) {
        errno = EAFNOSUPPORT;
        return -1;
    }

    addr->s_addr = string_to_ip((char*) src);
    return (addr->s_addr != 0);
}

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
const char *inet_ntop(int family, const void* src, char* dst, socklen_t size)
{
    const struct in_addr* addr = src;
    char* res;

    if (family != AF_INET) {
        errno = EAFNOSUPPORT;
        return NULL;
    }

    res = ip_to_string(addr->s_addr);
    if (size < strlen(res) + 1) {
        free(res);
        errno = ENOSPC;
        return NULL;
    }

    strncpy(dst, res, size);
    free(res);

    return dst;
}

struct hostent* gethostbyname(const char* name)
{
    static struct hostent hostent;
    static char* aliases[] = { NULL };
    static uint32_t ip;
    static uint32_t** h_addr_list = NULL;
    int i;

    // Also, falls h_addr_list nicht NULL ist... muessen wir mal bisschen
    // rumfreen
    for (i = 0; h_addr_list && h_addr_list[i]; i++) {
        free(h_addr_list[i]);
    }

    // Erstmal schauen, ob es eine IP-Adresse ist
    ip = string_to_ip(name);

    // Wenn nicht, muessen wir eine DNS-Anfrage machen
    if (ip != 0) {
        uint32_t* ip_q = malloc(sizeof(*ip_q));
        h_addr_list = realloc(h_addr_list, sizeof(uint32_t*) * 2);

        *ip_q = ip;
        h_addr_list[0] = ip_q;
        h_addr_list[1] = NULL;
    } else {
        char* path;
        FILE* f;
        unsigned long count = 0;

        // Datei oeffnen
        asprintf(&path, "tcpip:/dns/%s", name);
        f = fopen(path, "r");
        free(path);

        if (!f) {
            return NULL;
        }

        // Dateigroesse feststellen
        fseek(f, 0, SEEK_END);
        count = ftell(f);
        rewind(f);

        // Daten einlesen
        char ip_str[count + 1];
        fread(ip_str, 1, count, f);
        fclose(f);

        ip_str[count] = '\0';

        // An einem \n ist die IP-Adresse zu Ende
        char* p;
        p = strtok(ip_str, "\n");
        i = 0;
        while (p != NULL)
        {
            // IP in die Liste speichern
            uint32_t* ip_q = malloc(sizeof(uint32_t));
            *ip_q = string_to_ip(p);

            h_addr_list = realloc(h_addr_list, sizeof(uint32_t*) * (i + 2));
            h_addr_list[i] = ip_q;
            i++;

            // Nach \n aufteilen, naechste IP nehmen
            p = strtok(NULL, "\n");
        }

        // Wir muessen mit NULL terminieren
        h_addr_list[i] = NULL;
    }

    // hostent aktualisieren
    hostent.h_name = (char*) name;
    hostent.h_aliases = aliases;
    hostent.h_addrtype = AF_INET;
    hostent.h_length = 4;
    hostent.h_addr_list = (char**) h_addr_list;

    return &hostent;
}


/* Definition der bekannten Netzwerkdienste */
static char* service_aliases_http[] = { "http", NULL };
static char* service_aliases_imap[] = { "imap", NULL };
static char* service_aliases_irc[] = { "irc", NULL };

static struct servent services[] = {
    { "http",     service_aliases_http,   big_endian_word(80),    "tcp" },
    { "imap",     service_aliases_imap,   big_endian_word(143),   "tcp" },
    { "irc",      service_aliases_irc,    big_endian_word(6667),  "tcp" },
    { NULL, NULL, 0, NULL }
};

/**
 * Gibt Informationen zu einem Netzwerkdienst zurueck. Wenn der Dienst nicht
 * gefunden werden kann, wird NULL zurueckgegeben.
 *
 * @param name Name des Diensts (z.B. "imap")
 * @param protocol Name des Transportprotokolls (z.B. "tcp"). Falls nicht
 * angegeben, soll jeder Treffer zaehlen.
 */
struct servent* getservbyname(const char* name, const char* protocol)
{
    int i;
    for (i = 0; services[i].s_name != NULL; i++) {
        if (strcmp(name, services[i].s_name)) {
            continue;
        }
        if ((protocol == NULL) || strcmp(protocol, services[i].s_proto)) {
            return &services[i];
        }
    }

    return NULL;
}

/* Definition der bekannten Netzwerkprotokolle */
static char* proto_aliases_tcp[] = { "tcp", NULL };

static struct protoent protocols[] = {
    { "tcp",      proto_aliases_tcp,   6 },
    { NULL, NULL, 0 }
};

/**
 * Gibt Informationen zu einem Netzwerkprotokoll zurueck. Wenn das Protokoll
 * nicht gefunden werden kann, wird NULL zurueckgegeben.
 *
 * @param name Name des Protokolls (z.B. "tcp")
 */
struct protoent* getprotobyname(const char* name)
{
    int i;
    for (i = 0; protocols[i].p_name != NULL; i++) {
        if (!strcmp(name, protocols[i].p_name)) {
            return &protocols[i];
        }
    }

    return NULL;
}
