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

#include <types.h>
#include <lost/config.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <collections.h>

#include "kernel.h"
#include "mm.h"
#include "tasks.h"
#include "lock.h"
#include "timer.h"
#include "lostio/core.h"
#include "lostio/client.h"
#include "lostio/userspace.h"


struct on_destroy_info {
    pm_process_destroy_handler handler;
    void* prv;
};

/// Liste mit allen Prozessen
list_t* process_list;

/// Die naechste zu vergebende PID
static pid_t next_pid = 1;


extern void scheduler_init(void);
extern pm_process_t init_process;
extern pm_thread_t* idle_task;

/**
 * Implementierung des Idle-Task
 */
static void idle(void)
{
    while(1) {
        asm volatile("hlt");
    }
}

/**
 * Die Prozessverwaltung initialisieren
 */
void pm_init()
{
    interrupt_stack_frame_t* isf;

    // Prozessliste erstellen
    process_list = list_create();

    // Idle-Task erstellen
    idle_task = pm_thread_create(&init_process, &idle);
    isf = (interrupt_stack_frame_t*) idle_task->kernel_stack;
    isf->cs = 0x08;
    isf->ss = 0x10;
    isf->ds = 0x10;
    isf->es = 0x10;
    pm_scheduler_delete(idle_task);

    // Scheduler initialisieren
    scheduler_init();
}

/**
 * Eine PID fuer einen neuen Prozess generieren
 *
 * @return PID
 */
static pid_t generate_pid(void)
{ 
    // Spinlock, damit wir keine doppelten PIDs haben bei mehreren CPUs
    static lock_t pid_lock = 0;
    lock(&pid_lock);
    pid_t pid = next_pid++;
    unlock(&pid_lock);

    return pid;
}

static void parent_destroy_handler(pm_process_t* process, void* prv)
{
    pm_process_t* child = prv;
    BUG_ON(child->parent != process);
    child->parent = process->parent;
    if (child->parent) {
        pm_register_on_destroy(child->parent, parent_destroy_handler, child);
    }
}

/**
 * Prozess erstellen
 *
 * @param cmdline Kommandozeile (muss nullterminiert sein)
 *
 * @return Pointer auf die Prozssstruktur.
 */
pm_process_t* pm_create(pm_process_t* parent, const char* cmdline)
{
    pm_process_t* process = calloc(1, sizeof(pm_process_t));
    if (process == NULL) {
        return NULL;
    }

    // Eventhandlerlisten anlegen
    process->on_destroy = list_create();

    // Dem Prozess eine ID zuordnen
    process->pid = generate_pid();

    // Elternprozess zuordnen
    process->parent = parent;
    if (parent) {
        pm_register_on_destroy(parent, parent_destroy_handler, process);
    }

    // Kontext fuer den Prozess erstellen
    process->context = mmc_create();

    // Kommandozeile kopieren
    process->cmdline = malloc(strlen(cmdline) + 1);
    if (process->cmdline == NULL) {
        mmc_destroy(&process->context);
        free(process);
        return NULL;
    }
    memcpy(process->cmdline, cmdline, strlen(cmdline) + 1);
    
    // Threadliste initialisieren, damit der Scheduler gefahrlos darauf
    // zugreifen kann.
    process->threads = list_create();
    if (process->threads == NULL) {
        mmc_destroy(&process->context);
        free(process->cmdline);
        free(process);
        return NULL;
    }

    // Hier kommt der Stack des ersten Threads hin
    process->next_stack = (vaddr_t) USER_STACK_START;
    
    // Der Prozess ist so lange blockiert, bis er manuell entsperrt wird
    process->status = PM_STATUS_BLOCKED;

    // RPC initialisieren
    process->rpc_handler = NULL;
    process->rpcs = list_create();

    // Dateiliste initialisieren
    process->lio_streams =
        tree_create(struct lio_usp_stream, usp_item, usp_id);

    list_push(process_list, process);

    process->memory_used = 0;

    return process;
}

/**
 * Datenstrukturen eines Prozesses freigeben (falls von diesem Prozess selbst
 * aufgerufen, darf der Task noch nicht gewechselt worden sein, damit
 * unterbrechbare LIOv2-Funktionen benutzt werden koennen)
 *
 * @param process Pointer auf die Prozssstruktur.
 */
void pm_prepare_destroy(pm_process_t* process)
{
    // Alle Dateien schliessen und freigeben
    struct lio_usp_stream* fd = NULL;
    struct lio_usp_stream* next = NULL;

    fd = tree_next(process->lio_streams, NULL);
    while (fd != NULL) {
        next = tree_next(process->lio_streams, fd);
        // TODO Was machen mit Dateien, die sich nicht schließen lassen?
        (void) lio_usp_unref_stream(process, fd);
        fd = next;
    }
    tree_destroy(process->lio_streams);
}

/**
 * Prozess zerstoeren
 *
 * @param process Pointer auf die Prozssstruktur.
 */
void pm_destroy(pm_process_t* process)
{
    pm_process_t* parent = process->parent;
    struct on_destroy_info* info;
    int i;

    // Der aktuelle Speicherkontext darf nicht zum zu zerstoerenden Task
    // gehoeren, sonst kriegen wir Probleme beim freigeben des Page Directory
    if (&process->context == &mmc_current_context()) {
        mmc_activate(&init_process.context);
    }

    // Alle Threads loeschen
    pm_thread_t* thread;
    while ((thread = list_get_element_at(process->threads, 0))) {
        pm_thread_destroy(thread);
    }

    // Eventhandler ausfuehren
    while ((info = list_pop(process->on_destroy))) {
        info->handler(process, info->prv);
        free(info);
    }
    list_destroy(process->on_destroy);

    // Eigenen Eventhandler vom Elternprozess löschen
    if (parent) {
        pm_unregister_on_destroy(parent, parent_destroy_handler, process);
    }

    // IO-Ports freigeben
    io_ports_release_all(process);

    // Timer freigeben
    timer_cancel_all(process);

    // RPC-Strukturen freigeben
    // TODO Die einzelnen Listenglieder freigeben
    list_destroy(process->rpcs);
    process->rpcs = NULL;
    rpc_destroy_task_backlinks(process);

    // TODO Alle SHMs freigeben

    // Den Prozess aus der Liste entfernen
    pm_process_t* _proc;
    i = 0;
    while ((_proc = list_get_element_at(process_list, i))) {
        // Wenn der Prozess gefunden wurde, wird er jetzt aus der Liste
        // gelöscht.
        if (process == _proc) {
            list_remove(process_list, i);
            break;
        }

        i++;
    }

    // Kommandozeile freigeben
    free(process->cmdline);
    
    // Kontext freigeben
    mmc_destroy(&process->context);
    
    // TODO: Kernelstack freigeben
    
    // Zu guter letzt wird auch die Prozessstruktur freigegeben
    free(process);

    // Und schließlich noch den Elternprozess wieder aufwecken, falls er in
    // einem waitpid() stecken könnte.
    if (parent && parent->status == PM_STATUS_WAIT_FOR_RPC) {
        parent->status = PM_STATUS_READY;
    }
}

/**
 * Prozess sperren. Wenn der Prozess nicht erfolgreich gesperrt wurde, kann es
 * sein, das Threads gesperrt wurden, und nicht wieder ensperrt!
 *
 * @param process Pointer auf Prozessstruktur.
 *
 * @return true, wenn der Prozess erfolgreich gesperrt wurde, sonst false
 */
bool pm_block(pm_process_t* process)
{
    bool result = true;
    int i = 0;
    pm_thread_t* thread;

    // Die einzelnen Threads sperren
    while ((thread = list_get_element_at(process->threads, i++))) {
        if ((thread->status != PM_STATUS_BLOCKED)
            && pm_thread_block(thread) != true)
        {
            result = false;
        }
    }

    process->status = PM_STATUS_BLOCKED;

    return result;
}

/**
 * Prozess entsperren
 *
 * @param process Pointer auf Prozessstruktur.
 */
bool pm_unblock(pm_process_t* process)
{
    int i = 0;
    pm_thread_t* thread;

    // Die einzelnen Threads entsperren
    while ((thread = list_get_element_at(process->threads, i++))) {
        pm_thread_unblock(thread);
    }

    process->status = PM_STATUS_READY;

    return true;
}

/**
 * Prozess anhand seiner PID finden.
 *
 * @param pid PID
 *
 * @return Pointer auf Prozessstruktur oder NULL, wenn der Prozess nicht
 *          gefunden wurde.
 */
pm_process_t* pm_get(pid_t pid) {
    pm_process_t* process = NULL;
    int i = 0;
    // Liste solange durchsuchen, bis der Prozess gefunden wurde, oder das Ende
    // der Liste erreicht wurde. In diesem Fall ist process == NULL.
    while ((process = list_get_element_at(process_list, i))) {
        if (process->pid == pid) {
            break;
        }
        i++;
    }
    return process;
}

/**
 * Blockiert einen Task. Zu einem blockierten Task können keine RPC-Aufrufe
 * durchgeführt werden. Wenn der Task durch einen anderen Task blockiert ist,
 * wird er außerdem vom Scheduler nicht mehr aufgerufen, bis der Task
 * entblockt wird.
 *
 * @param task Zu blockierender Task
 * @param blocked_by PID des blockierenden Tasks
 */
bool pm_block_rpc(pm_process_t* task, pid_t blocked_by)
{
    if(task->blocked_by_pid == blocked_by) {
        task->blocked_count++;
        return true;
    }

    if (task->blocked_by_pid) {
        return false;
    }

    task->blocked_by_pid = blocked_by;
    task->blocked_count = 1;

    return true;
}

/**
 * Entblockt einen blockierten Task. Dies ist nur möglich, wenn der
 * aufrufende Task dem blockierenden Task entspricht
 *
 * @param task Zu entblockender Task
 * @param blocked_by Aufrufender Task
 *
 * @return true, wenn der Task erfolgreich entblockt wurde, false
 * im Fehlerfall.
 */
bool pm_unblock_rpc(pm_process_t* task, pid_t blocked_by)
{
    if (task->blocked_by_pid == blocked_by) {
        if(--(task->blocked_count) == 0) {
            task->blocked_by_pid = 0;
        }
        return true;
    } else {
        return false;
    }
}

/**
 * Handler fuer das Loeschen des Prozesses registrieren
 */
void pm_register_on_destroy(pm_process_t* process,
    pm_process_destroy_handler handler, void* prv)
{
    struct on_destroy_info* info = malloc(sizeof(*info));

    info->handler = handler;
    info->prv = prv;

    list_push(process->on_destroy, info);
}

/**
 * Handler fuer das Loeschen des Prozesses entfernen
 */
void pm_unregister_on_destroy(pm_process_t* process,
    pm_process_destroy_handler handler, void* prv)
{
    struct on_destroy_info* info;
    int i;

    for (i = 0; (info = list_get_element_at(process->on_destroy, i)); i++) {
        if (info->handler == handler && info->prv == prv) {
            list_remove(process->on_destroy, i);
            free(info);
            return;
        }
    }

    panic("pm_unregister_on_destroy: Handler war nicht registriert");
}
