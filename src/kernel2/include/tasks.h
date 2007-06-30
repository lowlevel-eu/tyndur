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

#ifndef _TASKS_H_
#define _TASKS_H_

#include <types.h>
#include <collections.h>

#include "mm.h"
#include "lock.h"

#define PM_STATUS_READY 0
#define PM_STATUS_BLOCKED 1
#define PM_STATUS_RUNNING 2

typedef struct {
    /// Die eindeutige Prozessnummer
    pid_t pid;

    /// Der Kontext fuer die Speicherverwaltung
    mmc_context_t context;

    /// Liste mit den Threads
    list_t* threads;

    /// Adresse, an der der naechste Thread seinen Stack ablegen soll
    vaddr_t next_stack;

    /// Kommandozeile die zum Starten des Prozesses benutzt wurde
    char* cmdline;

    /// Aktueller Status
    dword status;

    /// Wird gesperrt, wenn aenderungen am Prozess vorgenommen werden
    lock_t lock;
} pm_process_t;


typedef struct {
    /// Adresse des Kernelstack-Pointers
    vaddr_t kernel_stack;

    /// Anfang des Stacks
    vaddr_t kernel_stack_bottom;

    /// Groesse des Kernelstacks
    size_t kernel_stack_size;

    /// Der Prozess, dem der Thread gehoert
    pm_process_t* process;

    /// Aktueller Status
    dword status;

    /// Wird gesperrt, wenn aenderungen am Thread vorgenommen werden
    lock_t lock;
} pm_thread_t;


/**
 * Prozessverwaltung
 */
/// Prozessverwaltung initialisieren
void pm_init();

/// Prozess erstellen
pm_process_t* pm_create(vaddr_t entry, char* cmdline);

/// Prozess zerstoeren
void pm_destroy(pm_process_t* process);

/// Prozess blockieren
bool pm_block(pm_process_t* process);

/// Prozess entblocken
bool pm_unblock(pm_process_t* process);


/**
 * Threadverwaltung
 */
/// Neuen Thread erstellen
pm_thread_t* pm_thread_create(pm_process_t* process, vaddr_t entry);

/// Einen Thread zerstoeren
void pm_thread_destroy(pm_thread_t* thread);

/// Thread blockieren
bool pm_thread_block(pm_thread_t* thread);

/// Thread entblocken
bool pm_thread_unblock(pm_thread_t* thread);

/**
 * Scheduling
 */
/// Dem Scheduler einen neuen Thread hinzufuegen
void pm_scheduler_add(pm_thread_t* thread);

/// Einen Thread aus dem Scheduler entfernen
void pm_scheduler_delete(pm_thread_t* thread);

/// Die Liste im Scheduler aktualisieren
void pm_scheduler_refresh();

/// Einen Thread zum ausfuehren holen
pm_thread_t* pm_scheduler_pop();

/// Einen ausgefuerten Thread wieder zurueck an den Scheduler geben
void pm_scheduler_push(pm_thread_t* thread);

/**
 * Initialisierung
 */
/// Das Init-Modul laden
void load_init_module(struct multiboot_info* multiboot_info);

#endif //ifndef _TASKS_H_

