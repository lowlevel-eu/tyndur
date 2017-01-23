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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <readline/readline.h>
#include <types.h>

#define TMS_MODULE ftp
#include <tms.h>

#include "command.h"
#include "ftp.h"

char* host;
FILE* handle;

int main(int argc, char* argv[])
{
    tms_init();

    bool prgm_end = false;
    char* input;

    // nur ein Parameter ist gueltig
    if (argc == 2) {
        host = argv[1];
        ftp_connect(argv[1]);
    }

    // bildet den Ablauf des kompletten Programmes
    do {
        input = readline("\033[1;37mFTP> \033[0m");
        // durchlaeuft den String bis zum ersten Zeichen
        for (; isspace(*input); input++);

        if (!strncmp(input, "rm", 2)) {
            input += 2;
            input = parser(input);
            if (input != NULL) ftp_rm(input);
        } else if (!strncmp(input, "cd", 2)) {
            input += 2;
            input = parser(input);
            if (input != NULL) ftp_cd(input);
        } else if (!strncmp(input, "put", 3)) {
            input += 3;
            input = parser(input);
            if (input != NULL) ftp_put(input);
        } else if (!strncmp(input, "get", 3)) {
            input += 3;
            input = parser(input);
            if (input != NULL) ftp_get(input);
        } else if (!strncmp(input, "open", 4)) {
            input += 4;
            input = parser(input);
            host = input;
            if (input != NULL) ftp_connect(input);
        } else if (!strncmp(input, "mkdir", 5)) {
            input += 5;
            input = parser(input);
            if (input != NULL) ftp_mkdir(input);
        } else if (!strncmp(input, "rmdir", 5)) {
            input += 5;
            input = parser(input);
            if (input != NULL) ftp_rmdir(input);
        } else if (!strncmp(input, "bye", 3)) {
            ftp_disconnect();
            prgm_end = true;
        } else if (!strncmp(input, "quit", 4)) {
            ftp_disconnect();
            prgm_end = true;
        } else if (!strncmp(input, "help", 4)) {
            printf(TMS(usage,
                "FTP-Client Befehlsliste:\n\n"
                "ascii   - ascii Mode einschalten\n"
                "binary  - binary Mode einschalten\n"
                "bye     - schließt die Verbindung zum Server und beendet den FTP-Client\n"
                "cd      - [Pfad] wechseln des Verzeichnisses auf dem Server\n"
                "cdup    - wechselt zum Stammverzeichniss auf dem Server\n"
                "clear   - löscht den Inhalt des Terminals\n"
                "close   - schließt die Verbindung zum Server\n"
                "get     - [Pfad+Dateiname] holt eine Datei vom Server\n"
                "help    - zeigt diese Hilfe an\n"
                "mkdir   - [Pfad+Ordnername] erstellt einen Ordner auf dem Server\n"
                "open    - [ftp.name.net] öffnet eine Verbindung zu einem Server\n"
                "put     - [Pfad+Dateiname] speichert eine Datei auf dem Server\n"
                "pwd     - zeigt das akutell geoeffnete Verzeichniss auf dem Server an\n"
                "rm      - [Pfad+Dateiname] löscht die Datei auf dem Server\n"
                "rmdir   - [Pfad+Verzeichnisname] löscht ein Verzeichnis auf dem Server\n"
                "system  - zeigt den Namen des Betriebssystems vom Server an\n"
                "user    - einloggen mit Benutzername und Passwort\n")
            );
        } else if (!strncmp(input, "close", 5)) {
            ftp_disconnect();
        } else if (!strncmp(input, "user", 4)) {
            ftp_user();
        } else if (!strncmp(input, "system", 6)) {
            ftp_sys();
        } else if (!strncmp(input, "cdup", 4)) {
            ftp_cdup();
        } else if (!strncmp(input, "pwd", 3)) {
            ftp_pwd();
        } else if (!strncmp(input, "ls", 2)) {
            ftp_ls();
        } else if (!strncmp(input, "clear", 5)) {
            puts("\33[2J\33[H");
        } else if (!strncmp(input, "ascii", 5)) {
            // falls keine Verbindung besteht, abbrechen
            if (handle == NULL) {
                puts(TMS(no_connection, "\033[1;37mFehler: keine "
                    "Serververbindung vorhanden...\033[0m"));
                continue;
            }
            request("TYPE A");
        } else if (!strncmp(input, "binary", 6)) {
            if (handle == NULL) {
                puts(TMS(no_connection, "\033[1;37mFehler: keine "
                    "Serververbindung vorhanden...\033[0m"));
                continue;
            }
            request("TYPE I");
        } else {
            puts(TMS(type_help, "\033[1;37mGeben Sie help ein um eine "
                "Liste aller Befehle zu erhalten!\033[0m"));
        }

        free(input);
        } while (!prgm_end);
    return 0;
}
