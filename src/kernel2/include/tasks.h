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

#ifndef _TASKS_H_
#define _TASKS_H_

#include <types.h>
#include <collections.h>

#include "mm_arch.h"
#include "lock.h"

#define PM_STATUS_READY 0
#define PM_STATUS_BLOCKED 1
#define PM_STATUS_RUNNING 2
#define PM_STATUS_WAIT_FOR_RPC 3

typedef struct pm_process {
    /// Die eindeutige Prozessnummer
    pid_t pid;

    /// Elternprozess
    struct pm_process* parent;

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

    /// Der RPC-Handler fuer diesen Prozess
    vaddr_t rpc_handler;

    /// Die PID des Prozesses, der RPC fuer diesen Prozess abgestellt hat
    dword blocked_by_pid;

    /// Die Anzahl von p()s (RPC wird erst bei 0 wieder freigegeben)
    dword blocked_count;

    /// Eine Liste von RPC-Backlinks
    list_t* rpcs;

    /// Eine Liste von geoeffneten SHM-Bereichen
    list_t* shm;

    /**
     * Eine Liste von Eventhandlern, die beim Loeschen des Prozesses
     * aufgerufen werden
     */
    list_t* on_destroy;
} pm_process_t;

typedef struct {
    /// Adresse des Kernelstack-Pointers
    vaddr_t kernel_stack;

    /// Anfang des Stacks
    vaddr_t kernel_stack_bottom;

    /// Groesse des Kernelstacks
    size_t kernel_stack_size;

    /// Anfang des Usermode-Stacks
    vaddr_t user_stack_bottom;

    /// Der Prozess, dem der Thread gehoert
    pm_process_t* process;

    /// Aktueller Status
    dword status;

    /// Wird gesperrt, wenn aenderungen am Thread vorgenommen werden
    lock_t lock;
} pm_thread_t;

typedef void (*pm_process_destroy_handler)(pm_process_t* process, void* prv);

extern list_t* process_list;

/**
 * Prozessverwaltung
 */
/// Prozessverwaltung initialisieren
void pm_init(void);

/// Prozess erstellen
pm_process_t* pm_create(pm_process_t* parent, const char* cmdline);

/// Prozess zerstoeren
void pm_destroy(pm_process_t* process);

/// Prozess blockieren
bool pm_block(pm_process_t* process);

/// Prozess entblocken
bool pm_unblock(pm_process_t* process);

/// Prozess anhand seiner PID suchen
pm_process_t* pm_get(pid_t pid);

/// RPC-Empfang fuer einen Prozess blockieren
bool pm_block_rpc(pm_process_t* task, pid_t blocked_by);

/// RPC-Empfang fuer einen Prozess wieder freigeben
bool pm_unblock_rpc(pm_process_t* task, pid_t blocked_by);


/// Handler fuer das Loeschen des Prozesses registrieren
void pm_register_on_destroy(pm_process_t* process,
    pm_process_destroy_handler handler, void* prv);

/// Entfernt den gegebenen Task aus allen RPC-Backlinks
void rpc_destroy_task_backlinks(pm_process_t* destroyed_process);

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
void pm_scheduler_refresh(void);

/// Einen Thread zum ausfuehren holen
pm_thread_t* pm_scheduler_pop(void);

/// Einen ausgefuerten Thread wieder zurueck an den Scheduler geben
void pm_scheduler_push(pm_thread_t* thread);

/**
 * Initialisierung
 */
/// Das Init-Modul laden
void load_init_module(struct multiboot_info* multiboot_info);

/// Alle weiteren Module an init uebergeben
void load_multiboot_modules(struct multiboot_info* multiboot_info);

// Kann erst hier eingebunden werden, weil es die Strukturen braucht
#include "cpu.h"

#define current_thread (cpu_get_current()->thread)
#define current_process (cpu_get_current()->thread->process)

#endif //ifndef _TASKS_H_

