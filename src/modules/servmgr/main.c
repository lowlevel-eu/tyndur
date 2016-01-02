/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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

#include <stdint.h>
#include <syscall.h>
#include <collections.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <init.h>
#include <rpc.h>

#include "servmgr.h"

#define TMS_MODULE main
#include <tms.h>

bool service_running(const char* name);
bool service_wait(const char* name, uint32_t timeout);

const char* root_dir = "file:/";
const char* service_terminal = "";
bool config_done = false;

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

    if (argc > 2) {
        // Der erste Parameter ist wie unten beschreiben das Root-Verzeichnis
        root_dir = argv[1];
        chdir(root_dir);

        // Der zweite Parameter ist das Terminal das fuer die Ausgaben der
        // Services benutzt werden soll
        service_terminal = argv[2];

        // Die weiteren sind Services auf die gewartet werden muss.
        startup_services = list_create();
        for (i = 3; i < argc; i++) {
            list_push(startup_services, argv[i]);
        }
    } else {
        puts(TMS(usage, "Aufruf: servmgr <Root-Verzeichnis> <Service-Terminal>"
            "[Startup-Services ...]\n"));
        exit(-1);
    }
}

/** Dummy-Handle fuer Timer */
static void do_nothing(void) {}

/**
 * Auf Statup-Services warten
 *
 * @return true wenn alle gestartet werden konnten, false sonst.
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

        // Das System nicht mit Dauerpolling verstopfen
        timer_register(do_nothing, 50000);
        wait_for_rpc();
    }

    return (list_size(startup_services) == 0);
}

/**
 * Auf Root-Dateisystem warten
 *
 * @return true wenn das Dateisystem da ist, false sonst.
 */
static bool wait_root_dir(void)
{
    uint64_t timeout = get_tick_count() + 5000000;
    lio_resource_t res;
    lio_stream_t s;

    while (timeout > get_tick_count()) {
        res = lio_resource(root_dir, false);
        if (res >= 0) {
            goto found;
        }

        // Das System nicht mit Dauerpolling verstopfen
        timer_register(do_nothing, 50000);
        wait_for_rpc();
    }
    return false;

found:
    s = lio_open(res, LIO_SYMLINK | LIO_READ);
    if (s >= 0) {
        char buf[1024];
        int ret;
        ret = lio_read(s, sizeof(buf) - 1, buf);
        if (ret > 0) {
            buf[ret] = '\0';
            root_dir = strdup(buf);
            chdir(root_dir);
        }
        lio_close(s);
    }
    return true;
}

/**
 * Startup-Services anzeigen, die nicht gestartet wurden.
 */
static void failed_startup_services(void)
{
    const char* service;
    int i;

    for (i = 0; (service = list_get_element_at(startup_services, i)); i++) {
        printf(TMS(not_running, "Service '%s' läuft nicht!\n"), service);
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
    tms_init();

    // Parameter verarbeiten
    parse_params(argc, argv);

    // LostIO-Interfacee vorbereiten
    lioif_init();

    // IO fuer die Services initialisieren
    service_io_init();

    // RPC-Interface vorbereiten
    rpcif_init();

    // Auf startup-services warten
    if (!wait_startup_services()) {
        failed_startup_services();
        return EXIT_FAILURE;
    }

    // Auf Rootdateisystem warten
    if (!wait_root_dir()) {
        printf(TMS(no_root, "servmgr: Timeout beim Warten auf das "
                            "Rootdateisystem\n"));
        return EXIT_FAILURE;
    }

    // Konfiguration einlesen
    if (config_read()) {
        config_done = true;
    }

    // Services starten
    service_start("default");

    while (true) {
        wait_for_rpc();
    }
}

