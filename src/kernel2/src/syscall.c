/*
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
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
#include <stdint.h>

#include "syscall.h"
#include "syscallno.h"

#include "kernel.h"

syscall_t syscalls[SYSCALL_MAX];

/**
 * Die Syscalls initialisieren.
 */
void syscall_init()
{
    syscall_register(SYSCALL_MEM_ALLOCATE, &syscall_mem_allocate, 3);
    syscall_register(SYSCALL_MEM_FREE, &syscall_mem_free, 2);

    syscall_register(SYSCALL_PM_GET_PID, (void*) &syscall_pm_get_pid, 0);
    syscall_register(SYSCALL_PM_GET_PARENT_PID, (void*)
        &syscall_pm_get_parent_pid, 1);
    syscall_register(SYSCALL_PM_GET_CMDLINE, (void*) syscall_pm_get_cmdline,
        0);

    syscall_register(SYSCALL_SET_RPC_HANDLER, (void*) &syscall_set_rpc_handler,
        1);

    syscall_register(SYSCALL_PUTSN, (void*) &syscall_putsn, 2);
}

/**
 * Einen neuen Syscall registrieren
 *
 * @param number Syscallnummer (syscallno.h!)
 * @param handler Adresse der Handler-Funktion
 * @param arg_count Anzahl der Argumente, die fuer diesen Syscall uebergeben
 *                  werden muessen.
 */
void syscall_register(syscall_arg_t number, void* handler, size_t arg_count)
{
    // Ein Syscall mit einer zu grossen ID wuerde dazu fuehren, dass Teile von
    // Kernel-Daten ueberschrieben wuerden
    if (number >= SYSCALL_MAX) {
        panic("Es wurde versucht einen Syscall mit einer Nummer groesser als"
                "SYSCALL_MAX zu reservieren!");
    }
    syscalls[number].handler = handler;
    syscalls[number].arg_count = arg_count;
}
