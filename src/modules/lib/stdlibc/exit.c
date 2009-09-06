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

#include <syscall.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <collections.h>
#include <errno.h>
#include "init.h"

// Liste mit den Funktionen die ausgefuehrt werden sollen, sobald der Prozess
// beendet wird.
static list_t* atexit_list = NULL;

/**
 * Alle Puffer flushen und mit atexit gesetzte Funktion ausfuehren und den
 * Prozess anschliessend beenden.
 *
 * @param result Rueckgabewert des Prozesses
 */
void exit(int result)
{
    // Mit atexit registrierte Funktionen ausfuehren
    if (atexit_list != NULL) {
        int i;
        void (*function)(void);

        // Listeneintraege durchgehen und aufrufen
        for (i = 0; (function = list_get_element_at(atexit_list, i)); i++) {
            function();
        }
    }

    // TODO: Buffer
    _exit(result);
}

/**
 * Prozess abnormal beenden
 */
void abort()
{
    _exit(-1);
}

/**
 * Funktion registrieren, die beim Beenden ausgefuehrt werden soll
 */
int atexit(void (*function)(void))
{
    // Liste erstellen, falls sie nicht existiert
    if (atexit_list == NULL) {
        atexit_list = list_create();

        // Wenn das erstellen nicht klappt, fehlt ist nicht genug Speicher
        // vorhanden
        if (atexit_list == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }
    
    // Element an liste anhaengen
    list_push(atexit_list, function);
    return 0;
}

/**
 * Prozess beenden
 *
 * @param result Rueckgabewert des Prozesses
 */
void _exit(int result)
{
    // RPC an Elternprozess schicken
    uint8_t msg[12];
    char* function_name = (char*) msg;
    int* status = (int*) (msg + 8);

    strcpy(function_name, "CHL_EXIT");
    *status = result;

    send_message(get_parent_pid(0), 512, 0, 12, (char*) msg);

    init_process_exit(result);
    destroy_process();

    while(1);
}

