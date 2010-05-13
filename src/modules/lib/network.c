/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
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

#include <stdint.h>
#include <network.h>
#include <syscall.h>
#include <stdlib.h>
#include <string.h>
#include <init.h>
#include <rpc.h>

/**
 * Wandelt eine als String gegebene IP-Adresse in eine dword-Darstellung um,
 * wie sie von den meisten Netzwerkfunktionen ben�tigt wird.
 *
 * Erlaubtes Eingabeformat sind daf�r nur vier durch Punkte getrennte 
 * Dezimalzahlen (a.b.c.d). Andere Darstellungen (z.B. eine Hexzahl) sind
 * derzeit nicht unterst�tzt.
 */
uint32_t string_to_ip(const char* ip)
{
    uint8_t ip_bytes[4] = { 0, 0, 0, 0 };

    uint8_t cur_byte = 0;
    while (*ip) {
        switch (*ip) {
            case '.':
                if (++cur_byte >= 4) {
                    return 0;
                }
                break;
            
            case '0' ... '9':
                ip_bytes[cur_byte] *= 10;
                ip_bytes[cur_byte] += (*ip - '0');
                break;

            default:
                return *((uint32_t*) ip_bytes);
                break;
        }
        ip++;
    }

    return *((uint32_t*) ip_bytes);
}

/**
 * Wandelt eine IP-Adresse in einen String um
 */
char* ip_to_string(uint32_t ip)
{
    char* result;
    asprintf(&result, "%d.%d.%d.%d",
        ip & 0xFF,
        (ip >> 8) & 0xFF,
        (ip >> 16) & 0xFF,
        (ip >> 24) & 0xFF);

    return result;
}

char* mac_to_string(uint64_t mac)
{
    char *result;
    asprintf(&result, "%02x:%02x:%02x:%02x:%02x:%02x",
             (uint32_t) (mac & 0xFF),
             (uint32_t) ((mac >> 8) & 0xFF),
             (uint32_t) ((mac >> 16) & 0xFF),
             (uint32_t) ((mac >> 24) & 0xFF),
             (uint32_t) ((mac >> 32) & 0xFF),
             (uint32_t) ((mac >> 40) & 0xFF));
    return result;
}
    
/**
 * (Nur f�r Netzwerkkartentreiber)
 *
 * Registriert eine neue Netzwerkkarte zur Verwendung in der Implementierung
 * von Netzwerkprotokollen wie TCP/IP.
 *
 * @param device_number Beliebigr, aber innerhalb des Netzwerkkartentreibers
 * eindeutige ID f�r eine Netzwerkkarte. Andere Prozesse verwenden diese ID,
 * um auf eine bestimmte Netzwerkkarte zuzugreifen.
 *
 * @param max Die MAC-Adresse der Netzwerkkarte
 *
 * @param ip IP-Adresse, die der Netzwerkkarte zugeordnet ist.
 */
void register_netcard(uint32_t device_number, uint64_t mac)
{
    struct net_device driver = {
        .number = device_number,
        .mac = mac,
    };

    char buffer[8 + sizeof(struct net_device)];

    memcpy(buffer, RPC_IP_REGISTER_DRIVER, 8);
    memcpy(buffer + 8, &driver, sizeof(struct net_device));

    send_message(
        init_service_get("tcpip"),
        512,
        0,
        8 + sizeof(struct net_device),
        buffer
    );
}
