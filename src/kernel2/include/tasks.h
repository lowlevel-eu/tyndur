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

#include <stdint.h>
#include <stdbool.h>
#include <types.h>
#include <collections.h>
#include <lost/config.h>

#include "mm_arch.h"
#include "lock.h"
#include "multiboot.h"

#define PM_STATUS_READY 0
#define PM_STATUS_BLOCKED 1
#define PM_STATUS_RUNNING 2
#define PM_STATUS_WAIT_FOR_RPC 3
#define PM_STATUS_WAIT_FOR_LIO 4
#define PM_STATUS_WAIT_FOR_IO_REQUEST 5
#define PM_STATUS_WAIT_FOR_DATA 6

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
    uint32_t status;

    /// Wird gesperrt, wenn aenderungen am Prozess vorgenommen werden
    lock_t lock;

    /// Der RPC-Handler fuer diesen Prozess
    vaddr_t rpc_handler;

    /// Die PID des Prozesses, der RPC fuer diesen Prozess abgestellt hat
    uint32_t blocked_by_pid;

    /// Die Anzahl von p()s (RPC wird erst bei 0 wieder freigegeben)
    uint32_t blocked_count;

    /// Eine Liste von RPC-Backlinks
    list_t* rpcs;

    /// Eine Liste von geoeffneten SHM-Bereichen
    list_t* shm;

#if CONFIG_ARCH == ARCH_I386
    /// IO-Bitmap
    void* io_bitmap;
#elif CONFIG_ARCH == ARCH_AMD64
#else
#error Architektur nicht unterstuetzt
#endif

    /**
     * Eine Liste von Eventhandlern, die beim Loeschen des Prozesses
     * aufgerufen werden
     */
    list_t* on_destroy;

    /// Speicherverbrauch des Prozesses
    uintmax_t memory_used;

    /// Baum aller geoeffneten LostIO-Streams
    tree_t* lio_streams;
} pm_process_t;

typedef struct {
    /// Adresse des Kernelstack-Pointers
    vaddr_t kernel_stack;

    /// Anfang des Stacks
    vaddr_t kernel_stack_bottom;

    /// Groesse des Kernelstacks
    size_t kernel_stack_size;

    /// Aktueller Stackframe des Userspace-Interrupts
    vaddr_t user_isf;

    /// Anfang des Usermode-Stacks
    vaddr_t user_stack_bottom;

    /// Der Prozess, dem der Thread gehoert
    pm_process_t* process;

    /// Aktueller Status
    uint32_t status;

    /// Wird gesperrt, wenn aenderungen am Thread vorgenommen werden
    lock_t lock;

    /// Gesetzt, wenn der Thread ein VM86-Thread mit allem Drum und Dran ist
    bool vm86;

    /// Eindeutige Threadnummer
    tid_t tid;

    /// Gesetzt, wenn der Thread beim Beenden den gesamten Prozess beendet
    bool is_main_thread;

    /**
     * Eine Liste von Eventhandlern, die beim Loeschen des Threads
     * aufgerufen werden
     */
    list_t* on_destroy;

    /// Eine Liste der von diesem Thread angebotenen LIO-Services
    list_t* lio_services;

    /// Eine Liste aller LIO-Trees, deren Ring dieser Thread verarbeitet
    list_t* lio_trees;
} pm_thread_t;

typedef void (*pm_process_destroy_handler)(pm_process_t* process, void* prv);
typedef void (*pm_thread_destroy_handler)(pm_thread_t* thread, void* prv);

extern list_t* process_list;

/**
 * Prozessverwaltung
 */
/// Prozessverwaltung initialisieren
void pm_init(void);

/// Prozess erstellen
pm_process_t* pm_create(pm_process_t* parent, const char* cmdline);

/// Datenstrukturen eines Prozesses freigeben (vor dem Taskwechsel)
void pm_prepare_destroy(pm_process_t* process);

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
void pm_unregister_on_destroy(pm_process_t* process,
    pm_process_destroy_handler handler, void* prv);

/// Handler fuer das Loeschen des Threads registrieren
void pm_thread_register_on_destroy(pm_thread_t* thread,
    pm_thread_destroy_handler handler, void* prv);

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

/// Thread anhand der TID (und ggf. Prozess) suchen
pm_thread_t* pm_thread_get(pm_process_t* process, tid_t tid);

/// Thread entblocken
bool pm_thread_unblock(pm_thread_t* thread);

/// Darf der Thread einfach auf RUNNING gesetzt werden?
bool pm_thread_runnable(pm_thread_t* thread);

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

/// Einen bestimmten Thread zum Ausfuehren holen
void pm_scheduler_get(pm_thread_t* thread);

/// Einen ausgefuerten Thread wieder zurueck an den Scheduler geben
void pm_scheduler_push(pm_thread_t* thread);

/// Kontrolle vom aktuellen Kernelthread an einen anderen Thread abgeben
void pm_scheduler_yield(void);

/// Kontrolle an einen bestimmten Thread abgeben, mit Folgezustand
void pm_scheduler_kern_yield(tid_t tid, int status);

/// Versucht einen Taskwechsel zum übergebenen Thread
void pm_scheduler_try_switch(pm_thread_t* thread);

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

