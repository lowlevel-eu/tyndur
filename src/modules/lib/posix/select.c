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

#include <sys/select.h>
#include <sys/time.h>
#include <stdio.h>
#include <syscall.h>

#ifndef CONFIG_LIBC_NO_STUBS
/**
 * Prueft, welche der gegebenen Dateideskriptoren bereit zum Lesen oder
 * Schreiben sind oder ein Fehler fuer sie aufgetreten ist. Dateideskriptoren
 * die nicht bereit bzw. in einem Fehlerzustand sind, werden aus der Menge
 * entfernt.
 *
 * @param number_fds Nummer des hoechsten Dateideskriptors in einem der
 * uebergebenen Mengen.
 * @param readfds Dateideskriptoren, bei denen ueberprueft werden soll, ob sie
 * zum Lesen bereit sind.
 * @param writefds Dateideskriptoren, bei denen ueberprueft werden soll, ob sie
 * zum Schreiben bereit sind.
 * @param errfds Dateideskriptoren, bei denen ueberprueft werden soll, ob sie
 * in einem Fehlerzustand sind.
 * @param timeout Maximales Timeout, das gewartet werden soll, falls kein
 * Deskriptor bereit ist. NULL fuer dauerhaftes Blockieren.
 *
 * @return Anzahl der Dateideskriptoren, die bereit bzw. in einem Fehlerzustand
 * sind
 */
int select(int number_fds, fd_set* readfds, fd_set* writefds,
    fd_set* errfds, struct timeval* timeout)
{
    uint32_t* rd = (uint32_t*) readfds;
    uint32_t* wr = (uint32_t*) writefds;
    uint32_t* err = (uint32_t*) errfds;
    int c;
    FILE* f;
    int ret;

    uint32_t orig_rd = rd ? *rd : 0;
    uint32_t orig_wr = wr ? *wr : 0;

    uint64_t timeout_tick;
    if (timeout != NULL) {
        timeout_tick = get_tick_count() + timeout->tv_sec +
            1000ULL * 1000ULL * timeout->tv_usec;
    } else {
        timeout_tick = (uint64_t) -1;
    }

    // tyndur ist so toll, bei uns gibt es keine Fehler
    if (err) {
        *err = 0;
    }

    do {
        int i;

        // Wieder alles zuruecksetzen
        ret = 0;
        if (rd) {
            *rd = orig_rd;
        }
        if (wr) {
            *wr = orig_wr;
        }


        for (i = 0; i < 32; i++) {

            // Versuchsweise ein Zeichen auslesen. Wenn das funktioniert, als
            // lesbar werten und das Zeichen schnell wieder zurueckstecken
            // bevor das Programm was merkt.
            if (rd && (*rd & (1 << i))) {
                f = fdopen(i, NULL);

                if (f != NULL) {
                    c = fgetc(f);
                    if (c == 0 || c == EOF) {
                        *rd &= ~(1 << i);
                    } else {
                        ungetc(c, f);
                        ret++;
                    }
                }
            }

            // Schreiben geht immer
            if (wr && (*wr & (1 << i))) {
                ret++;
            }
        }

        // Busy Wait waere irgendwie auch doof
        if (ret == 0) {
            yield();
        }

    } while ((ret == 0) && (timeout_tick > get_tick_count()));

    return ret;
}
#endif
