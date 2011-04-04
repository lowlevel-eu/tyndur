/**
 * Copyright (c) 2009, 2011 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Paul Lange.
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
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCZLUDING, BUT NOT LIMITED
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <types.h>
#include <ctype.h>
#include <string.h>
#include <readline/readline.h>

#define TMS_MODULE ftp
#include <tms.h>

#include "ftp.h"

#define COMMAND_PORT 21
#define LINE_LENGTH 80


/**
 * parst die Zweitangabe aus dem Eingabestring, da die Leerzeichen
 * stoeren
 *
 * @param string zeigt auf den Eingabestring
 * @return gibt Pointer auf die Zweitangabe zurueck
 */
char* parser(char* string)
{
    char* buffer;

    for (; isspace(*string); string++);
    if (*string != '\0') {
        for (buffer = string; !isspace(*buffer) && *buffer; buffer++);
        *buffer = '\0';
        return string;
    }
    return NULL;
}


/**
 * sendet einen Befehl an den FTP-Server und gibt den Request aus
 *
 * @param command enthaelt den Befehl für den FTP-Server
 * @return Statuscode
 */
int request(const char* command)
{

    fprintf(handle, "%s\r\n", command);
    fflush(handle);

    return response();
}


/**
 * gibt den Requeststring auf dem Bildschirm aus
 *
 * @return Statuscode
 */
int response(void)
{
    char code[5];

    fgets(code, 5, handle);
    printf("%s", code);

    bool oneLine = code[3] == ' ';

    code[3] = ' '; code[4] = '\0';

    char buffer[LINE_LENGTH];

    while(fgets(buffer, LINE_LENGTH, handle) != NULL) {
        printf("%s", buffer);

        bool lastLine = oneLine || strncmp(buffer, code, 4) == 0;

        size_t resp_size = strlen(buffer);
        while (buffer[resp_size-1] != '\n') {

            if (fgets(buffer, LINE_LENGTH, handle) == NULL) {
                lastLine = true;
                break;
            }

            resp_size = strlen(buffer);
            printf("%s", buffer);
        }

        if (lastLine)
            break;
    }

    return atoi(code);
}


/**
 * Stellt eine Verbindung zum FTP-Server her
 *
 * @param hostname Name des Hostes
 */
void ftp_connect(char* hostname)
{
    char* host_path;
    char* prompt;
    char* input;
    char* c;

    if (handle != NULL) return;
    puts(TMS(connecting, "\033[1;37mVerbinde...\033[0m"));

    // tcpip-Pfad erzeugen
    asprintf(&host_path, "tcpip:/%s:%d", hostname, COMMAND_PORT);
    handle = fopen(host_path, "r+");
    free(host_path);

    if (!handle) {
        puts(TMS(host_not_reachable, "\033[1;37mFehler: Host nicht "
            "erreichbar!\033[0m"));
        handle = NULL;
        return;
    }
    printf(TMS(connected, "\033[1;37mVerbunden mit %s, warte auf "
        "Antwort...\033[0m\n"), host);
    response();

    asprintf(&prompt, TMS(prompt_user, "\033[1;37mBenutzer (%s): \033[0m"),
        host);
    input = readline(prompt);
    free(prompt);

    // sorgt dafür, dass die Leerzeichen entfernt werden
    for (; isspace(*input); input++);
    for (c = input; !isspace(*c) && *c; c++);
    *c = '\0';

    fprintf(handle, "USER %s\r\n", input);
    fflush(handle);
    if (response() == 331) {

        input = readline(TMS(prompt_password, "\033[1;37mPasswort: \033[0m"));

        for (; isspace(*input); input++);
        for (c = input; !isspace(*c) && *c; c++);
        *c = '\0';

        fprintf(handle, "PASS %s\r\n", input);
        fflush(handle);
           response();
    }

    request("TYPE A");
}


/**
 * Schliest die Verbindung zum FTP-Server
 */
void ftp_disconnect(void)
{
    if (handle == NULL) return;

    request("QUIT");
    fclose(handle);
    handle = NULL;
    puts(TMS(disconnected, "\033[1;37mVerbindung getrennt...\033[0m"));
}


/**
 * Oeffnet eine passive Datenverbindung zum FTP-Server
 *
 * @return Handle zum Datenfluss
 */
FILE* ftp_data_connect(void)
{
    char buffer;
    char* c, *host_path;
    char response[LINE_LENGTH], portI[4], portII[4];
    bool line_end = true;
    unsigned int port, port_low;
    int i = 0;
    FILE* data_handle = NULL;

    puts(TMS(data_connect, "\033[1;37mDatenverbindung wird geöffnet..."
        "\033[0m"));
    fprintf(handle, "%s\r\n", "PASV");
    fflush(handle);

    // hier wird statt response() aufzurufen, der Inhalt dieser Funktion
    // wiedergegeben, da der Inhalt des response noch gebraucht wird
    while (line_end) {
        while (((buffer = fgetc(handle)) != '\n') && i < (LINE_LENGTH - 1)) {
            if (buffer != EOF) {
               response[i++] = buffer;
            }
        }
        response[i] = '\0';
        printf("%s\n", response);
        line_end = false;

        if ((i > 3) && (response[3] == '-')) {
            line_end = true;
            i = 0;
        }
    }
    // bis zum zur Klammer Parsen
    for (c = response; *c != '(' || *c == '\n'; c++);

    // hier wird der Port rausgeparst, der dann noch in Zahlenwerte umgewandelt
    // werden sowie noch zusammengerechnet werden muss, da der Port so vom
    // Server verschickt wird '(127, 0, 0, 1, p1,p2)'. Vorne die IP dahinter
    // der Port, der so zusammengerechnet werden muss p1*256 + p2!
    i = 0;
    while (i < 4) {
        for (; *(c++) != ',' || *c == '\n';);
        i++;
    }

    i = 0;
    while (*c != ',' || *c == '\n') {
        portI[i] = *c;
        i++;
        c++;
    }
    portI[i++] = '\0';
    c++;
    i = 0;
    while (*c != ')' || *c == '\n') {
         portII[i] = *c;
         i++;
         c++;
    }
    portII[i++] = '\0';
    port = atoi(portI);
    port *= 256;
    port_low = atoi(portII);
    port += port_low;

    asprintf(&host_path, "tcpip:/%s:%d", host, port);
    data_handle = fopen(host_path, "r+");
    free(host_path);

    return data_handle;
}
