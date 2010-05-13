/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
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

#include <stdint.h>
#include <collections.h>
#include <stdlib.h>

#include "syscall.h"

static list_t* timers;
static uint32_t next_timer_id = 0;

struct timeout {
    void (*callback)(void);
    uint32_t timer_id;
};

uint32_t timer_register(void (*callback)(void), uint32_t usec)
{
    uint32_t id;

    // Initialisieren der Liste
    if (!timers) {
        timers = list_create();
    }

    // Anlegen des Timeout-Objekts
    struct timeout* timeout = malloc(sizeof(struct timeout));

    timeout->callback   = callback;
    timeout->timer_id   = next_timer_id++;

    // An der richtigen Stelle in der Liste einsortieren
    p();
    struct timeout* item;
    int i;
    for (i = 0; (item = list_get_element_at(timers, i)); i++) {
        if (item->timer_id > timeout->timer_id) {
            break;
        }
    }
    list_insert(timers, i, timeout);

    // Timer beim Kernel registrieren
    syscall_timer(timeout->timer_id, usec);

    id = timeout->timer_id;
    v();

    return id;
}

void timer_callback(uint32_t timer_id)
{
    struct timeout* item;
    int i;
    
    p();
    for (i = 0; (item = list_get_element_at(timers, i)); i++) {
        if (item->timer_id == timer_id) {
            list_remove(timers, i);
            item->callback();
            free(item);
            v();
            return;
        }
    }
    v();
}

void timer_cancel(uint32_t timer_id)
{
    struct timeout* item;
    int i;
    
    p();
    for (i = 0; (item = list_get_element_at(timers, i)); i++) {
        if (item->timer_id == timer_id) {
            list_remove(timers, i);
            free(item);
        }
    }
    v();
}
