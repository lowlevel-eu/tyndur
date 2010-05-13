/*
 * Copyright (c) 2006-2007 The tyndur Project. All rights reserved.
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

#include <types.h>
#include <collections.h>
#include <stdlib.h>

#include "timer.h"
#include "rpc.h"

extern uint64_t timer_ticks;


static list_t* timers;

struct timeout {
    struct task*    task;
    uint32_t           timer_id;
    uint64_t           timeout;
};

void timer_init()
{
    timers = list_create();
}

void timer_register(struct task* task, uint32_t timer_id, uint32_t usec)
{
    // Anlegen des Timeout-Objekts
    struct timeout* timeout = malloc(sizeof(struct timeout));

    timeout->task       = task;
    timeout->timer_id   = timer_id;
    timeout->timeout    = timer_ticks + usec;

    // An der richtigen Stelle in der Liste einsortieren, so dass immer nur
    // das erste Listenelement geprüft werden muss
    struct timeout* item;
    int i;
    for (i = 0; (item = list_get_element_at(timers, i)); i++) {
        if (item->timeout > timeout->timeout) {
            break;
        }
    }

    list_insert(timers, i, timeout);
}

void timer_notify(uint64_t microtime)
{
    static uint64_t last_microtime = 0;
    static const uint32_t rpc_timer_function = 514;
    
    struct timeout* item;
    for (; (item = list_get_element_at(timers, 0)); free(list_pop(timers))) 
    {
        // Die Schleife soll nur so lange laufen, wie die Timeouts in der 
        // Vergangenheit liegen. Da die Liste sortiert ist, kann beim ersten
        // Timeout, der in der Zukunft liegt, abgebrochen werden.
        //
        // Overflows müssen abgefangen werden, da in diesem Spezialfall eine
        // größere Zahl dennoch Vergangenheit bedeutet. Wie ich grad sehe,
        // reden wir von einem qword und damit von einer Uptime von ein paar
        // hunderttausend Jahren, aber LOST ist ja stabil und wir daher
        // optimistisch.
        if (((item->timeout > microtime) && (microtime > last_microtime)) 
            || ((item->timeout < last_microtime) && (microtime < last_microtime)))
        {
            break;
        }

        // Task per RPC informieren, dass der Timer abgelaufen ist
        // Im Fehlerfall mit der Abarbeitung der Liste aufhören und es beim
        // nächsten Mal nochmal versuchen.
        //
        // FIXME Mit p(); while(1); kann ein Prozess den Timer für das ganze
        // System lahmlegen
        if (!fastrpc(item->task, 
                4, (char*) &rpc_timer_function, 
                4, (char*) &item->timer_id)) 
        {
            break;
        }
    }

    last_microtime = microtime;
}

void timer_cancel_all(struct task* task)
{
    struct timeout* item;
    int i;
    for (i = 0; (item = list_get_element_at(timers, i)); i++) {
        if (item->task == task) {
            list_remove(timers, i);
        }
    }
}
