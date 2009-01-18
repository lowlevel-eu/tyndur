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
#include <collections.h>

#include "tasks.h"
#include "lock.h"
#include "timer.h"


struct on_destroy_info {
    pm_process_destroy_handler handler;
    void* prv;
};

/// Liste mit allen Prozessen
list_t* process_list;

/// Die naechste zu vergebende PID
static pid_t next_pid = 1;


extern void scheduler_init(void);

/**
 * Die Prozessverwaltung initialisieren
 */
void pm_init()
{
    // Prozessliste erstellen
    process_list = list_create();
    
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

    list_push(process_list, process);

    return process;
}

/**
 * Prozess zerstoeren
 *
 * @param process Pointer auf die Prozssstruktur.
 */
void pm_destroy(pm_process_t* process)
{
    struct on_destroy_info* info;

    // Den Prozess blockieren
    while (pm_block(process) == FALSE);

    // Eventhandler ausfuehren
    while ((info = list_pop(process->on_destroy))) {
        info->handler(process, info->prv);
        free(info);
    }
    list_destroy(process->on_destroy);

    // Timer freigeben
    timer_cancel_all(process);

    // RPC-Strukturen freigeben
    // TODO Die einzelnen Listenglieder freigeben
    list_destroy(process->rpcs);

    // TODO Alle SHMs freigeben

    // Den Prozess aus der Liste entfernen
    pm_process_t* _proc;
    int i = 0;
    while ((_proc = list_get_element_at(process_list, i))) {
        // Wenn der Prozess gefunden wurde, wird er jetzt aus der Liste
        // gelöscht.
        if (process == _proc) {
            list_remove(process_list, i);
            break;
        }

        i++;
    }

    // Alle Threads loeschen
    i = 0;
    pm_thread_t* thread;
    while ((thread = list_get_element_at(process->threads, i++))) {
        pm_thread_destroy(thread);
    }
    
    // Kommandozeile freigeben
    free(process->cmdline);
    
    // Kontext freigeben
    mmc_destroy(&process->context);
    
    // TODO: Kernelstack freigeben
    
    // Zu guter letzt wird auch die Prozessstruktur freigegeben
    free(process);
}

/**
 * Prozess sperren. Wenn der Prozess nicht erfolgreich gesperrt wurde, kann es
 * sein, das Threads gesperrt wurden, und nicht wieder ensperrt!
 *
 * @param process Pointer auf Prozessstruktur.
 *
 * @return TRUE, wenn der Prozess erfolgreich gesperrt wurde, sonst FALSE
 */
bool pm_block(pm_process_t* process)
{
    bool result = TRUE;
    int i = 0;
    pm_thread_t* thread;

    // Die einzelnen Threads sperren
    while ((thread = list_get_element_at(process->threads, i++))) {
        if ((thread->status != PM_STATUS_BLOCKED)
            && pm_thread_block(thread) != TRUE)
        {
            result = FALSE;
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

    return TRUE;
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
        return TRUE;
    }

    if (task->blocked_by_pid) {
        return FALSE;
    }

    task->blocked_by_pid = blocked_by;
    task->blocked_count = 1;

    return TRUE;
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
        return TRUE;
    } else {
        return FALSE;
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
