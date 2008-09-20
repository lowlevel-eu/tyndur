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

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <types.h>
#include <stdint.h>

#include "cpu.h"

// Ist das wirklich die korrekte Definition? Dieser Typ sollte der Groesse der
// Werte die auf dem Stack gespeichert werden entsprechen.
// FIXME: Doppelte Definition! (modules/include/syscall.h)
typedef uintptr_t syscall_arg_t;

typedef struct {
    /// Pointer auf den Handler des Syscalls oder NULL, falls dieser Syscall
    /// nicht existiert.
    void* handler;
    
    /// Anzahl der Argumente die der Syscall benoetigt
    size_t arg_count;
} syscall_t;

/// In diesem Array werden alle Syscalls gespeichert.
extern syscall_t syscalls[];


/// Architektur-Abhaengiger Handler, der die eigentlichen Handler aufruft
void syscall_arch(machine_state_t* isf);


/// Einen neuen Syscall verfuegbar machen
void syscall_register(syscall_arg_t number, void* handler, size_t arg_count);

/// Syscalls initialisieren
void syscall_init();

// ###########################################################################
// SYSCALLS
// ###########################################################################

// Speicherverwaltung
/// Einen Speicherblock allokieren.
vaddr_t syscall_mem_allocate(size_t bytes, syscall_arg_t flags, paddr_t phys);

/// Einen Speicherblock freigeben
void syscall_mem_free(vaddr_t start, size_t bytes);

// Prozessverwaltung
/// PID des aktuellen Prozesses abfragen
pid_t syscall_pm_get_pid();

/// PID des Elternprozesses ausfindig machen
pid_t syscall_pm_get_parent_pid(pid_t pid);

/// Befehlszeile des aktuellen Prozesses abfragen
const char* syscall_pm_get_cmdline();

/// Kritischen Abschnitt betreten
int syscall_pm_p(void);

/// Kritischen Abschnitt verlassen
int syscall_pm_v(pid_t pid);

/// Kritischen Abschnitt verlassen und auf RPC warten
int syscall_pm_v_and_wait_for_rpc();

/// Die Kontrolle an einen anderen Task abgeben
void syscall_pm_sleep(void);

/// Warten, bis ein RPC zu bearbeiten ist
void syscall_pm_wait_for_rpc(void);

/// Prozess erstellen
pid_t syscall_pm_create_process(vaddr_t start, uid_t uid,
    const char* cmdline, pid_t parent_pid);

/// Speicher an einen anderen Prozess uebergeben
void syscall_init_child_page(pid_t pid, vaddr_t src, vaddr_t dest,
    size_t size);

/// IO-Ports anfordern
void syscall_io_request_port(uint32_t port, uint32_t length);

/// Aktuelle Zeit abfragen (Mikrosekunden seit Systemstart)
uint64_t syscall_get_tick_count(void);


// RPC
/// RPC-Handler registrieren
void syscall_set_rpc_handler(vaddr_t address);

/// RPC durchfuehren
int syscall_fastrpc(pid_t callee_pid, size_t metadata_size, void* metadata,
    size_t data_size, void* data);

/// Von einem RPC zurueckkehren
void syscall_fastrpc_ret(void);

/// Interrupt registrieren
void syscall_add_interrupt_handler(uint32_t intr);

// Diverse
/// Textausgabe ueber den Kernel
void syscall_putsn(int char_count, char* source);

#endif //ifndef _SYSCALL_H_

