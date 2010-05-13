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
#include <types.h>
#include <collections.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Konfiguration eines Dienstes
 */
struct config_service {
    char* name;
    char* start_cmd;
    list_t* deps;
    bool running;

    struct {
        enum {
            SERVSTART,
            TERMINATE,
            NOWAIT
        } wait;
    } conf;
};


extern const char* root_dir;
extern bool config_done;


/**
 * PID eines Services
 *
 * @param name Name des Services
 *
 * @return PID wenn der Service Laeuft, 0 sonst
 */
pid_t service_pid(const char* name);

/**
 * Pruefen ob ein bestimmter Service laeuft.
 *
 * @param name Name des Services
 *
 * @return TRUE wenn er laeuft, FALSE sonst
 */
bool service_running(const char* name);

/**
 * Warten bis ein bestimmter Service gestartet wurde.
 *
 * @param name Service Name
 * @param timeout Timeout in Millisekunden
 *
 * @return TRUE wenn der Service innerhalb dieser Zeit gestartet wurde oder
 *         schon lief, FALSE sonst.
 */
bool service_wait(const char* name, uint32_t timeout);

/**
 * Service starten
 *
 * @param name Service Name
 *
 * @return TRUE wenn der Service erfolgreich gestartet wurde, FALSE sonst
 */
bool service_start(const char* name);


/**
 * IO fuer Services vorbereiten
 */
void service_io_init(void);

/**
 * Ausgaben von Services verarbeiten. Die Ausgaben werden gepuffert und in
 * regelmaessigen Abstaenden geflusht.
 *
 * @param buf   Puffer mit den Ausgaben
 * @param size  Anzahl der Zeichen im Puffer
 */
void service_io_write(const char* buf, size_t size);

/**
 * IO-Puffer flushen
 */
void service_io_flush(void);


/**
 * Aufhoeren, Ausgaben auch auf die Kernelkonsole weiterzuleiten
 */
void service_io_stop_kputsn(void);


/**
 * Konfiguration der Services einlesen
 */
bool config_read(void);

/**
 * Konfiguration fuer einen Service holen
 */
struct config_service* config_service_get(const char* name);



/**
 * RPC-Interface initialisieren
 */
void rpcif_init(void);


/**
 * LostIO-Interface initialisieren
 */
void lioif_init(void);
