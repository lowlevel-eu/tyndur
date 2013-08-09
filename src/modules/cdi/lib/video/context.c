/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Alexander Siol.
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
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#include "video.h"
#include <types.h>
#include <cdi/lists.h>
#include "context.h"
#include <stdlib.h>

int context_ctr = 0;
cdi_list_t context_list;
cdi_list_t user_list;

void context_initialize(void)
{
    context_list = cdi_list_create();
    user_list = cdi_list_create();
}

driver_context_t* get_current_context_of_pid(pid_t pid)
{
    int i;
    for (i = 0; i < cdi_list_size(user_list); i++) {
        pid_context_t* pidcontext;
        pidcontext = cdi_list_get(user_list, i);
        if (pidcontext->owner == pid) {
            return pidcontext->context;
        }
    }
    return NULL;
}

int set_current_context_of_pid(pid_t pid, driver_context_t *context)
{
    int i;
    // Zuordnung suchen
    for (i = 0; i < cdi_list_size(user_list); i++) {
        pid_context_t* pidcontext;
        pidcontext = cdi_list_get(user_list, i);
        if (pidcontext->owner == pid) {
            // Gefunden, ersetzen
            pidcontext->context = context;
            return 0;
        }
    }

    // Keine Zuordnung gefunden, neue anlegen
    pid_context_t* pidcontext = calloc(1, sizeof(pid_context_t));
    pidcontext->owner = pid;
    pidcontext->context = context;
    cdi_list_push(user_list, pidcontext);

    return 0;
}

driver_context_t* get_context_by_id(int id)
{
    int i;
    for (i = 0; i < cdi_list_size(context_list); i++) {
        driver_context_t* context;
        context = cdi_list_get(context_list, i);
        if (context->id == id) {
            return context;
        }
    }
    return NULL;
}

driver_context_t* create_context(pid_t pid)
{
    driver_context_t *context = calloc(1, sizeof(driver_context_t));
    context->owner = pid;
    context->id = ++context_ctr;

    cdi_list_push(context_list, context);

    return context;
}
