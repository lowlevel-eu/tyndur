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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rpc.h>
#include <syscall.h>

#include "servmgr.h"

#define FLUSH_DELAY 1


/// Pfad zum Ausgabeterminal
extern const char* service_terminal;

/// Hande fuer das Ausgabeterminal
static FILE* servterm = NULL;

/// Ausgabepuffer
static char* out_buffer = NULL;

/// Anzahl der Zeichen im Ausgabepuffer
static size_t out_buffer_pos = 0;

/// Ausgaben auf Kernelkonsole kopieren?
static int use_kputsn = 1;


/**
 * IO fuer Services vorbereiten
 */
void service_io_init(void)
{
    send_message(1, RPC_MESSAGE, 0, 8, "UP_STDIO");
}

/**
 * Ausgaben von Services verarbeiten. Die Ausgaben werden gepuffert und in
 * regelmaessigen Abstaenden geflusht.
 *
 * @param buf   Puffer mit den Ausgaben
 * @param size  Anzahl der Zeichen im Puffer
 */
void service_io_write(const char* buf, size_t size)
{
    p();

    // Wenn das Terminal noch nicht bereit ist, puffern wir die Daten noch ein
    // Bisschen
    if (!servterm) {
        // Puffer vergroessern und Daten kopieren
        out_buffer = realloc(out_buffer, out_buffer_pos + size);
        memcpy(out_buffer + out_buffer_pos, buf, size);
        out_buffer_pos += size;

        // Solange vterm noch nicht da ist, geben wir es auch noch ueber den
        // Kernel aus
        if (use_kputsn) {
            syscall_putsn(size, buf);
        }
    } else {
        fwrite(buf, 1, size, servterm);
        fflush(servterm);
    }

    v();
}

/**
 * IO-Puffer flushen
 */
void service_io_flush(void)
{
    FILE* f = servterm;

    // Terminal oeffnen falls es noch nicht geoeffnet ist
    if (!f && !(f = fopen(service_terminal, "w"))) {
        timer_register(service_io_flush, 1000000 * FLUSH_DELAY);
        goto out;
    }

    // Pufferinhalt in das Terminal schreiben
    p();
    if (!out_buffer || !fwrite(out_buffer, 1, out_buffer_pos, f)) {
        goto out;
    }
    fflush(f);

    free(out_buffer);
    out_buffer = NULL;
    out_buffer_pos = 0;

out:
    v();
    servterm = f;
}

/**
 * Aufhoeren, Ausgaben auch auf die Kernelkonsole weiterzuleiten
 */
void service_io_stop_kputsn(void)
{
    use_kputsn = 0;
}
