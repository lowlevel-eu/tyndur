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

#ifndef _NETDB_H_
#define _NETDB_H_

struct hostent {
        char*   h_name;
        char**  h_aliases;
        int     h_addrtype;
        int     h_length;
        char**  h_addr_list;
#define h_addr h_addr_list[0]
};

struct servent {
    char*   s_name;
    char**  s_aliases;

    /// Portnummer des Diensts, in Big Endian
    int     s_port;

    char*   s_proto;

};

struct protoent {
    char*   p_name;
    char**  p_aliases;
    int     p_proto;
};


#ifdef __cplusplus
extern "C" {
#endif

struct hostent* gethostbyname(const char* name);

/**
 * Gibt Informationen zu einem Netzwerkdienst zurueck. Wenn der Dienst nicht
 * gefunden werden kann, wird NULL zurueckgegeben.
 *
 * @param name Name des Diensts (z.B. "imap")
 * @param protocol Name des Transportprotokolls (z.B. "tcp"). Falls nicht
 * angegeben, soll jeder Treffer zaehlen.
 */
struct servent* getservbyname(const char* name, const char* protocol);

/**
 * Gibt Informationen zu einem Netzwerkprotokoll zurueck. Wenn das Protokoll
 * nicht gefunden werden kann, wird NULL zurueckgegeben.
 *
 * @param name Name des Protokolls (z.B. "tcp")
 */
struct protoent* getprotobyname(const char* name);

/** Fehlerstatus fuer gethostbyaddr() und gethostbyname() */
extern int h_errno;

/** Werte fuer h_errno */
enum {
    HOST_NOT_FOUND,
    NO_DATA,
    NO_RECOVERY,
    TRY_AGAIN,
};

/**
 * Gibt den Fehlerstring fuer einen gegebenen Wert von h_errno zurueck
 */
const char* hstrerror(int error);

/**
 * Gibt str gefolgt vom String fuer h_errno auf stderr aus. Wenn str NULL oder
 * ein leerer String ist, wird nur der Fehlerstring ausgegeben.
 */
void herror(const char* str);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif

