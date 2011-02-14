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
#include <stdbool.h>
#include <syscall.h>
#include <init.h>
#include <collections.h>
#include <sys/wait.h>

#include "servmgr.h"

#define TMS_MODULE service
#include <tms.h>

/**
 * Pid eines Services anhand seines Namens finden
 *
 * @param name Name
 *
 * @return Pid
 */
pid_t service_pid(const char* name)
{
    // FIXME init_service_get sollte ein const haben
    return init_service_get((char*) name);
}

/**
 * Pruefen ob ein Service laeuft
 *
 * @param name Name
 *
 * @return true wenn er laeuft, false sonst
 */
bool service_running(const char* name)
{
    return (service_pid(name) != 0);
}

/**
 * Warten bis der Service laeuft
 *
 * @param name Name
 * @param timeout Anzahl der Millisekunden die gewartet werden soll, bis
 *                aufgegeben wird
 *
 * @return true wenn der Service laeuft
 */
bool service_wait(const char* name, uint32_t timeout)
{
    uint64_t end = get_tick_count() + timeout * 1000;
    bool running;

    while (!(running = service_running(name)) && (end > get_tick_count())) {
        yield();
    }

    return running;
}

/**
 * Service aus der Konfiguration starten, mit Beruecksichtigung der
 * Abhaengigkeiten
 *
 * @param name Name des Services
 *
 * @return true wenn der Service gestartet werden konnte, false sonst
 */
bool service_start(const char* name)
{
    struct config_service* conf_serv = config_service_get(name);
    struct config_service* dep;
    int i;

    if (service_running(name) || (conf_serv && conf_serv->running)) {
        return true;
    }

    if (!conf_serv) {
        printf(TMS(not_found, "servmgr: Service nicht gefunden: '%s'\n"), name);
        return false;
    }

    // Abhaengigkeiten starten
    for (i = 0; (dep = list_get_element_at(conf_serv->deps, i)); i++) {
        if (!service_start(dep->name)) {
            printf(TMS(dependency, "servmgr: Konnte die Abhängigkeit von %s "
                "nicht erfüllen. '%s' kann deshalb nicht gestartet werden.\n"),
                dep->name, name);
            return false;
        }
    }

    if (conf_serv->start_cmd) {
        pid_t pid = init_execute(conf_serv->start_cmd);
        if ((conf_serv->conf.wait == SERVSTART) && !service_wait(name, 10000)) {
            printf(TMS(timeout, "servmgr: Timeout beim Warten auf den "
                "Service '%s'\n"), name);
            return false;
        } else if (conf_serv->conf.wait == TERMINATE) {
            int status;
            waitpid(pid, &status, 0);
        }
    }
    conf_serv->running = true;
    return true;
}
