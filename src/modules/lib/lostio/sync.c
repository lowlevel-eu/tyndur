/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
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
/** \addtogroup LostIO
 * @{
 */

#include <stdint.h>
#include "types.h"
#include "rpc.h"
#include "collections.h"
#include "lostio_internal.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

/// In dieser Struktur werden die Informationen zu aufgeschobenen reads
/// gespeichert
struct sync_read_s {
    lostio_filehandle_t* filehandle;
    io_read_request_t read_request;
    uint32_t correlation_id;
    pid_t pid;
};

static list_t* sync_read_list = NULL;

/**
 * Verarbeitet eventuell vorhandene aufgeschobene Anfragen fuer synchrones
 * Lesen.
 */
void lostio_sync_dispatch()
{
    int i = 0;
    struct sync_read_s* sync_read;

    // Wenn noch keine Anfragen vorhanden sind, wird abgebrochen.
    if (sync_read_list == NULL) {
        return;
    }

    while ((sync_read = list_get_element_at(sync_read_list, i)) != NULL) {
        // Pruefen ob jetzt genug Daten vorhanden sind.
        if ((sync_read->filehandle->node->size - sync_read->filehandle->pos) >=
            (sync_read->read_request.blocksize * sync_read->read_request.
            blockcount))
        {
            rpc_io_read(sync_read->pid, sync_read->correlation_id,
                sizeof(io_read_request_t), &sync_read->read_request);

            list_remove(sync_read_list, i);
        } else {
            i++;
        }
    }
}

/**
 * Schiebt eine Anfrage fuer synchrones Lesen auf, falls keine Daten vorhanden
 * sind.
 *
 * @param filehandle Filehandle von dem gelesen werden soll
 * @param read_request Gesendete anfrage
 */
void lostio_sync_read_wait(lostio_filehandle_t* filehandle, pid_t pid,
    uint32_t correlation_id, io_read_request_t* read_request)
{
    struct sync_read_s* sync_read = malloc(sizeof(struct sync_read_s));

    if (sync_read_list == NULL) {
        sync_read_list = list_create();
    }

    sync_read->pid = pid;
    sync_read->correlation_id = correlation_id;

    // Read-Request kopieren, da er beim zurueckspringen aus dem rpc-Handler
    // zerstoert wird.
    memcpy(&sync_read->read_request, read_request, sizeof(io_read_request_t));

    sync_read->filehandle = filehandle;

    sync_read_list = list_push(sync_read_list, sync_read);
}

