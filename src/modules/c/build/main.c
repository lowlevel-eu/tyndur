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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>

#include "build.h"

#define TMS_MODULE main
#include <tms.h>

/**
 * Legt Kuerzel fuer Dateitypen fest, die fuer die Anzeige benutzt werden
 * koennen, waehrend die Datei verarbeitet wird
 */
char* filetype_str[] = {
    [OTHER_FILE]    = " ? ",
    [SUBDIR]        = "DIR",
    [OBJ]           = "OBJ",
    [LANG_C]        = " C ",
    [LANG_PAS]      = "PAS",
    [LANG_ASM_GAS]  = "ASM",
    [LANG_ASM_NASM] = "ASM",
};

/**
 * Legt die aktuelle Architektur fest.
 *
 * Wird benutzt, falls arch/-Verzeichnisse im Projekt existieren
 */
char* arch = "i386";

/**
 * Mit verbose != 0 wird die Kommandozeile aller aufgerufenen Tools angezeigt.
 * Sonst wird nur eine kurze Statusmeldung ausgegeben.
 */
int verbose = 0;

/// Nur Meldungen ausgeben, aber nichts wirklich aufrufen
int dry_run= 0;

/// Nur Bibliotheken sammeln und linken
int dont_compile= 0;

/// Keine Standardbibliotheken einbinden
int standalone = 0;

static void usage(char* binary)
{
    fprintf(stderr, TMS(usage, "Aufruf: %s [-k] [-nc] [-v] [--dry-run] "
        "[<Wurzelverzeichnis>]\n"), binary);
    exit(1);
}

int main(int argc, char** argv)
{
    struct build_dir* rootdir;
    int i;
    char* rootdir_path = NULL;
    uint64_t start_time;
    uint64_t msecs;
    uint64_t secs;

    tms_init();

    start_time = get_tick_count();

    // Verarbeitung der Kommandozeilenparameter
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-v")) {
            verbose = 1;
        } else if (!strcmp(argv[i], "--dry-run")) {
            dry_run = 1;
        } else if (!strcmp(argv[i], "-k")) {
            standalone = 1;
        } else if (!strcmp(argv[i], "-nc")) {
            dont_compile = 1;
        } else if (argv[i][0] == '-') {
            usage(argv[0]);
        } else if (rootdir_path) {
            usage(argv[0]);
        } else {
            rootdir_path = argv[i];
        }
    }

    // Ins Wurzelverzeichnis des Projekts wechseln
    if (rootdir_path) {
        if (chdir(argv[i])) {
            fprintf(stderr, TMS(dir_not_found, "Verzeichnis nicht gefunden\n"));
            return 1;
        }
    }

    // Alle zu kompilierenden Dateien, Includeverzeichnisse usw. erfassen
    rootdir = scan_directory(NULL, ".");
    if (rootdir == NULL) {
        fprintf(stderr, TMS(dir_not_found, "Verzeichnis nicht gefunden\n"));
        return 1;
    }

    // Bauen!
    build(rootdir);

    // Aufraeumen
    free_directory(rootdir);

    msecs = (get_tick_count() - start_time) / 1000;
    secs = msecs / 1000;
    msecs -= secs * 1000;
    printf(TMS(runtime, "Laufzeit: %lld Sec. %lld Msec.\n"), secs, msecs);
    return 0;
}
