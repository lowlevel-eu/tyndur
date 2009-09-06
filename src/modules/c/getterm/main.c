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
#include <rpc.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdint.h>
#include <sleep.h>
#include <string.h>
#include <sys/wait.h>
#include <init.h>


// Aufruf an Console zum aendern der Terminal-Optionen
typedef enum {STDIN, STDOUT, STDERR} console_handle_t;
typedef struct {
    pid_t pid;
    console_handle_t type;
    char path[];
} __attribute__((packed)) console_ctrl_t;

/**
 * Pfad fuer Terminal aendern
 */
static void console_set_handle(console_handle_t handle, const char* path)
{
    size_t path_len = strlen(path);
    size_t size = sizeof(console_ctrl_t) + path_len + 1;
    
    // Buffer vorbereiten
    uint8_t buffer[size];
    console_ctrl_t* ctrl = (console_ctrl_t*) buffer;

    // Befehlsstruktur auffuellen
    ctrl->pid = -1;
    ctrl->type = handle;
    memcpy(ctrl->path, path, path_len + 1);
    
    // RPC durchfuehren
    pid_t console_pid = init_service_get("console");
    dword result = rpc_get_dword(console_pid, "CONS_SET",
        size, (char*) buffer);
    if (result == 0) {
        printf("Fehler beim setzen des Pfades fuer die I/O-Handles.");
        exit(-1);
    }
}

/**
 * Hauptfunktion
 */
int main(int argc, char* argv[])
{
    const char* program;

    // Wenn die Anzahl der Argumente nicht stimmt wird abgebrochen
    if (argc != 5) {
        puts("Aufruf: getterm <stdin> <stdout> <stderr> <Programm>");
        return -1;
    }
    program = argv[4];

    // Pfade bei console aendern
    console_set_handle(STDIN, argv[1]);
    console_set_handle(STDOUT, argv[2]);
    console_set_handle(STDERR, argv[3]);

    // Lokale Ein- und Ausgabehandles auch anpassen
    stdio_init();

    while (TRUE) {
        char input = 0;
        printf("\n\nEingabetaste druecken um %s zu starten...\n", program);
        // Auf Druecken der Eingabetaste warten
        while((fread(&input, 1, 1, stdin) != 1) || (input != '\n')) {
            yield();
        }

        pid_t pid = init_execute(program);
            
        // Fehler ist aufgetreten
        if (pid == 0) {
            printf("Fehler beim ausfuehren von '%s'\n", program);
            return -1;
        }

        // Jetzt wird gewartet, bis der Prozess terminiert
        waitpid(pid, NULL, 0);
    }

    return 0;
}

