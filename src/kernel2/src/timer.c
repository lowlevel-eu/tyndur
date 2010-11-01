/*
 * Copyright (c) 2006-2009 The tyndur Project. All rights reserved.
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

#include <stdint.h>
#include <stdbool.h>
#include <collections.h>
#include <stdlib.h>
#include <ports.h>

#include "timer.h"
#include "syscall.h"

extern uint64_t timer_ticks;
extern int do_fastrpc(pid_t callee_pid, size_t metadata_size, void* metadata,
    size_t data_size, void* data, bool syscall);

/**
 * Liste aller registrierten Timer
 */
static list_t* timers;

/**
 * Repraesentiert einen registrierten Timer. Ein Timer gehoert zun einem
 * bestimmten Prozess und nach Ablauf der vorgegebenen Zeitspanne wird ein RPC
 * zu diesem Prozess durchgefuehrt
 */
struct timeout {
    pm_process_t*   task;
    uint32_t        timer_id;
    uint64_t        timeout;
};

#define PIT_COUNTER_0       (0x0 << 6)
#define PIT_RW_LO_BYTE      (0x1 << 4)
#define PIT_RW_HI_BYTE      (0x2 << 4)
#define PIT_RW_FULL         (0x3 << 4)
#define PIT_MODE_2_RATE_GEN (0x2 << 1)

/**
 * Initialisiert die Timerverwaltung
 */
void timer_init(void)
{
    uint16_t reload_value;

    timers = list_create();

    // PIT initialisieren
    reload_value = 1193182 / CONFIG_TIMER_HZ;
    outb(0x43, PIT_COUNTER_0 | PIT_RW_FULL | PIT_MODE_2_RATE_GEN);
    outb(0x40, reload_value & 0xff);
    outb(0x40, reload_value >> 8);
}

/**
 * Registriert einen neuen Timer
 */
void timer_register(pm_process_t* task, uint32_t timer_id, uint32_t usec)
{
    // Anlegen des Timeout-Objekts
    struct timeout* timeout = malloc(sizeof(struct timeout));

    timeout->task       = task;
    timeout->timer_id   = timer_id;
    timeout->timeout    = timer_ticks + usec;

    // An der richtigen Stelle in der Liste einsortieren, so dass immer nur
    // das erste Listenelement geprueft werden muss
    struct timeout* item;
    int i;
    for (i = 0; (item = list_get_element_at(timers, i)); i++) {
        if (item->timeout > timeout->timeout) {
            break;
        }
    }

    list_insert(timers, i, timeout);
}

/**
 * Aufzurufen bei jeder Aktualisierung der Systemzeit. Fuehrt die
 * RPC-Benachrichtigungen fuer alle faelligen Timer aus.
 */
void timer_notify(uint64_t microtime)
{
    static uint64_t last_microtime = 0;
    static const uint32_t rpc_timer_function = 514;

    struct timeout* item;
    int i = 0;
    while ((item = list_get_element_at(timers, i)))
    {
        // Die Schleife soll nur so lange laufen, wie die Timeouts in der
        // Vergangenheit liegen. Da die Liste sortiert ist, kann beim ersten
        // Timeout, der in der Zukunft liegt, abgebrochen werden.
        //
        // Overflows muessen abgefangen werden, da in diesem Spezialfall eine
        // groessere Zahl dennoch Vergangenheit bedeutet. Wie ich grad sehe,
        // reden wir von einem qword und damit von einer Uptime von ein paar
        // hunderttausend Jahren, aber tyndur ist ja stabil und wir daher
        // optimistisch.
        if (((item->timeout > microtime) && (microtime > last_microtime))
            || ((item->timeout < last_microtime) && (microtime < last_microtime)))
        {
            break;
        }

        // Task per RPC informieren, dass der Timer abgelaufen ist.
        // Im Fehlerfall ueberspringen und es beim naechsten Mal nochmal
        // versuchen.
        if (do_fastrpc(item->task->pid,
                4, (char*) &rpc_timer_function,
                4, (char*) &item->timer_id, false))
        {
            i++;
            continue;
        }

        free(list_remove(timers, i));
    }

    last_microtime = microtime;
}

/**
 * Entfernt alle registrierten Timer eines Prozesses
 */
void timer_cancel_all(pm_process_t* task)
{
    struct timeout* item;
    int i;
    for (i = 0; (item = list_get_element_at(timers, i)); i++) {
        if (item->task == task) {
            list_remove(timers, i);
        }
    }
}
