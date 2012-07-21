/*
 * Copyright (c) 2012 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Patrick Pokatilo.
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
#include <stdlib.h>
#include "syscall.h"

/**
 * ID des aktuellen Threads abfragen.
 *
 * @return ID
 */
tid_t get_tid(void)
{
    tid_t pid;

    asm(
        "mov %1, %%eax;"
        "int $0x30;"
        : "=a" (pid) : "i" (SYSCALL_PM_GET_TID));

    return pid;
}

/**
 * Neuen Prozess erstellen. Dieser ist solange blockiert, bis er mit
 * unblock_task erloest wird. Waerend dieser Zeit kann der Elternprozess Mit
 * Hilfe von init_child_page Einzelne Seiten in den Adressraum des neuen
 * Prozesses mappen.
 *
 * @param initial_eip Der Einsprungspunkt an dem der Prozess seine Arbeit
 *                      beginnen soll.
 * @param uid Die Benutzernummer unter der der Neue Prozess laufen soll (FIXME:
 *              Sollte vom Elternprozess geerbt werden)
 * @param args Pointer auf die Kommandozeilen-Parameter
 * @param parent Prozessnummer des Elternprozesses, dem der neue Task
 *          untergeordenet werden soll, oder 0 fuer den aktuellen Prozess.
 *
 * @return Prozessnummer
 */
typedef struct
{
    void *(*func)(void *);
    void *arg;
} ct_parameter_t;

void *ct_start(ct_parameter_t *p)
{
    void *result = p->func(p->arg);

    free(p);

    exit_thread();

    return result;
}

tid_t create_thread(uint32_t start, void *arg)
{
    tid_t tid;

    ct_parameter_t *ct_arg = malloc(sizeof(ct_parameter_t));

    ct_arg->func = (void *(*)(void *))start;
    ct_arg->arg = arg;

    asm(
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x08, %%esp;"
        : "=a" (tid)
        : "i" (SYSCALL_PM_CREATE_THREAD), "r" (ct_start), "r" (ct_arg));

    return tid;
}

void exit_thread(void)
{
    asm(
        "mov %0, %%eax;"
        "int $0x30;"
        : : "i" (SYSCALL_PM_EXIT_THREAD));
}
