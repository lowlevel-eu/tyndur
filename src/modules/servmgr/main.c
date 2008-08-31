/*
 * Copyright (c) 2008 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Antoine Kaufmann.
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
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
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

#include <stdint.h>
#include <syscall.h>
#include <collections.h>
#include <unistd.h>
#include <stdlib.h>
#include <init.h>

#include "servmgr.h"

bool service_running(const char* name);
bool service_wait(const char* name, uint32_t timeout);

const char* root_dir = "file:/";
bool config_done = FALSE;

// Liste mit den Services die gestartet werden muessen, bevor die Konfiguration
// ausgelesen werden kann, die Services werden daraus geloescht, sobald sie
// laufen
list_t* startup_services;

/**
 * Parameter fuer den Programmaufruf verarbeiten
 *
 * @param argc Anzahl der Parameter
 * @param argv Pointer auf die Argumente
 */
static void parse_params(int argc, char* argv[])
{
    int i;

    if (argc > 1) {
        // Der erste Parameter ist wie unten beschreiben das Root-Verzeichnis
        root_dir = argv[1];
        chdir(root_dir);

        // Die weiteren sind Services auf die gewartet werden muss.
        startup_services = list_create();
        for (i = 2; i < argc; i++) {
            list_push(startup_services, argv[i]);
        }
    } else {
        puts("servmgr: Bitte Rootverzeichnis als ersten Parameter angeben!");
        exit(-1);
    }
}

/**
 * Auf Statup-Services warten
 *
 * @return TRUE wenn alle gestartet werden konnten, FALSE sonst.
 */
static bool wait_startup_services(void)
{
    const char* service;
    int i;
    uint64_t timeout = get_tick_count() +
        (uint64_t) list_size(startup_services) * 5000000;

    // Alle Startupservices abarbeiten und sie aus der Liste loeschen sobald sie
    // laufen
    while ((list_size(startup_services) > 0) && (timeout > get_tick_count())) {
        for (i = 0; (service = list_get_element_at(startup_services, i)); i++) {
            if (service_running(service)) {
                list_remove(startup_services, i--);
            }
        }
    }

    return (list_size(startup_services) == 0);
}

/**
 * Startup-Services anzeigen, die nicht gestartet wurden.
 */
static void failed_startup_services(void)
{
    const char* service;
    int i;

    for (i = 0; (service = list_get_element_at(startup_services, i)); i++) {
        printf("Service '%s' laeuft nicht!\n", service);
    }
}

/**
 * Hauptfunktion
 *
 * Vorgesehene Parameter fuer den Programmaufruf:
 *  - Root-Verzeichnis (Konfiguration in $ROOT/config/servmgr/)
 *  - Services auf die gewartet werden muss
 */
int main(int argc, char* argv[])
{
    // Parameter verarbeiten
    parse_params(argc, argv);

    // RPC-Interface vorbereiten
    rpcif_init();

    // Auf startup-services warten
    if (!wait_startup_services()) {
        failed_startup_services();
        return EXIT_FAILURE;
    }

    // Konfiguration einlesen
    if (!config_read()) {
        return EXIT_FAILURE;
    }
    config_done = TRUE;

    // Services starten
    service_start("default");

    while (TRUE) {
        wait_for_rpc();
    }
}

