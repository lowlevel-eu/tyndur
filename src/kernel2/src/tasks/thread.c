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
#include <string.h>
#include <collections.h>

#include "tasks.h"
#include "mm.h"
#include "lock.h"
#include "cpu.h"

struct on_destroy_info {
    pm_thread_destroy_handler handler;
    void* prv;
};

static tid_t next_tid = 1;

static tid_t generate_tid(void)
{
    static lock_t tid_lock = LOCK_UNLOCKED;
    lock(&tid_lock);
    tid_t tid = next_tid++;
    unlock(&tid_lock);

    return tid;
}

/**
 * Neuen Thread erstellen
 *
 * @param process Pointer auf Prozessstruktur
 * @param entry Einstrpungsadresse
 *
 * @return Pointer auf Threadstruktur
 */
pm_thread_t* pm_thread_create(pm_process_t* process, vaddr_t entry)
{
    // Speicher fuer die Thread-Struktur allozieren, wenn das misslingt wird
    // abgebrochen.
    pm_thread_t* thread = calloc(1, sizeof(pm_thread_t));
    if (thread == NULL) {
        return NULL;
    }

    thread->is_main_thread = (process != current_process);

    // Dem Thread eine ID zuordnen
    thread->tid = generate_tid();
    
    // Ein Pointer auf den Prozess im Thread hinterlegen
    thread->process = process;

    // Eventhandlerlisten anlegen
    thread->on_destroy = list_create();
    
    // Der Lock wird gesperrt initialisiert
    thread->lock = 1;

    // Normalerweise sind Threads keine VM86-Threads
    thread->vm86 = false;

    // Eine Page fuer den Kernelstacks des Threads allozieren
    thread->kernel_stack_size = 0x1000;
    paddr_t kernel_stack_phys = pmm_alloc(NUM_PAGES(thread->
        kernel_stack_size));

    // Den Stack mappen und einen pointer auf den ISF erstellen
    thread->kernel_stack_bottom = vmm_kernel_automap(kernel_stack_phys, 
        NUM_PAGES(thread->kernel_stack_size));
    thread->kernel_stack = (vaddr_t) ((uintptr_t) thread->kernel_stack_bottom +
        thread->kernel_stack_size - sizeof(interrupt_stack_frame_t));
    interrupt_stack_frame_t* isf = (interrupt_stack_frame_t*) thread->
        kernel_stack;
    thread->user_isf = thread->kernel_stack;

    //  Wir wollen alle Register schoen mit 0 initialisiert haben
    memset(isf, 0, sizeof(interrupt_stack_frame_t));

    #if CONFIG_ARCH == ARCH_I386
        isf->esp = (uint32_t) process->next_stack;
        isf->eip = (uint32_t) entry;
        
        // Flags: Interrupts aktivieren, und PL fuer IO-Operationen auf 3
        // setzen.
        isf->eflags = 0x0202;
        
        isf->cs = 0x1B;
        isf->ds = 0x23;
        isf->es = 0x23;
        isf->fs = 0x23;
        isf->gs = 0x23;
        isf->ss = 0x23;

        thread->user_stack_bottom = (vaddr_t) (isf->esp - USER_STACK_SIZE);
    #else
        isf->rsp = (uint64_t) process->next_stack;
        isf->rip = (uint64_t) entry;

        thread->user_stack_bottom = (vaddr_t) (isf->rsp - USER_STACK_SIZE);
    #endif

    /* TODO Irgendeine Art von Größenbeschränkung mit Guard Page, dass der
     * Stack nicht in den nächsten Stack reinwächst  */
    process->next_stack = (vaddr_t) ((uintptr_t) process->next_stack - 
        USER_MAX_STACK_SIZE);
    
    // Page fuer den Userspace-Stack allozieren
    paddr_t stack_phys = pmm_alloc(NUM_PAGES(USER_STACK_SIZE));

    mmc_map(&process->context, thread->user_stack_bottom,
        stack_phys, MM_FLAGS_USER_DATA, NUM_PAGES(USER_STACK_SIZE));

    // Ein leerer Thread verarbeitet keine LIO-Trees
    thread->lio_trees = NULL;

    // Thread in die Liste aufnehmen, und beim Scheduler regisrieren
    list_push(process->threads, thread);
    
    // Ab hier ist der Thread lauffaehig
    thread->status = PM_STATUS_READY;
    unlock(&thread->lock);

    pm_scheduler_add(thread);

    return thread;
}

/**
 * Thread anhalten und zerstoeren
 *
 * @param thread Pointer auf die Thread-Struktur
 */
void pm_thread_destroy(pm_thread_t* thread)
{
    struct on_destroy_info* info;
    // TODO Andere Threads beenden, falls thread->is_main_thread

    // Zuerst muss der Thread angehalten werden
    while (pm_thread_block(thread) == false);
    
    // Thread sperren
    lock(&thread->lock);

    // Beim Scheduler abmelden
    pm_scheduler_delete(thread);

    // Eventhandler ausfuehren
    while ((info = list_pop(thread->on_destroy))) {
        info->handler(thread, info->prv);
        free(info);
    }
    list_destroy(thread->on_destroy);

    // Wir brauchen immer einen gueltigen aktiven Thread, sonst kommt der
    // Interrupthandler durcheinander und kann bei Exceptions im Kernel keine
    // ordentliche Meldung mehr anzeigen.
    if (current_thread == thread) {
        current_thread = pm_scheduler_pop();
    }

    // TODO: Den Userspacestack auch freigeben

    // Kernelstack freigeben
    /*
     * FIXME Wieder freigeben, und dieses Mal dann auch physisch
    mmc_unmap(&thread->process->context, thread->kernel_stack_bottom,
        NUM_PAGES(thread->kernel_stack_size));
    */
    
    // Aus der Threadliste enfernen
    pm_thread_t* thread_;
    int i = 0;
    lock(&thread->process->lock);
    while ((thread_ = list_get_element_at(thread->process->threads, 
        i)))
    {
        if (thread_ == thread) {
            list_remove(thread->process->threads, i);
            break;
        }
        i++;
    }
    unlock(&thread->process->lock);

    // Zum abschliessen wird noch die Thread-Struktur freigegeben
    free(thread);
}

/**
 * Thread innerhalb eines Prozesses suchen
 *
 * @process Prozess, zu dem der gesuchte Thread gehört
 * @tid Thread-ID des Threads
 *
 * @return Der Thread mit der passenden TID, oder NULL, wenn der Thread nicht
 * existiert oder nicht zum gegebenen Prozess gehört.
 */
static pm_thread_t* pm_thread_get_in_proc(pm_process_t* process, tid_t tid)
{
    pm_thread_t* thread;
    int i;

    for (i = 0; (thread = list_get_element_at(process->threads, i)); i++) {
        if (thread->tid == tid) {
            return thread;
        }
    }

    return NULL;
}

/**
 * Thread anhand der TID suchen
 *
 * @process Prozess, zu dem der gesuchte Thread gehört, oder NULL, wenn in
 *          allen Prozessen gesucht werden soll
 *
 * @tid     Thread-ID des Threads
 *
 * @return Der Thread mit der passenden TID, oder NULL, wenn der Thread nicht
 * existiert oder nicht zum gegebenen Prozess gehört.
 */
pm_thread_t* pm_thread_get(pm_process_t* process, tid_t tid)
{
    pm_thread_t* thread;
    int i;

    if (process) {
        return pm_thread_get_in_proc(process, tid);
    }

    for (i = 0; (process = list_get_element_at(process_list, i)); i++) {
        thread = pm_thread_get_in_proc(process, tid);
        if (thread) {
            return thread;
        }
    }

    return NULL;
}

/**
 * Thread blockieren
 *
 * @param thread Pointer auf die Thread-Struktur
 *
 * @return true, wenn der Thread gesperrt wurde, sonst false
 */
bool pm_thread_block(pm_thread_t* thread)
{
    // TODO: In SMP-Systemen sollte hier ein IPI kommen, falls der Thread auf
    // einer anderen CPU am laufen ist, der sie dazu zwingt, den Thread
    // abzugeben.

    // Wenn der Thread gesperrt ist, muss garnicht weiter probiert werden
    if (locked(&thread->lock) == true) {
        return false;
    }

    lock(&thread->lock);

    if (thread->status != PM_STATUS_RUNNING) {
        thread->status = PM_STATUS_BLOCKED;
    }

    unlock(&thread->lock);

    return (thread->status == PM_STATUS_BLOCKED);
}

/**
 * Thread entsperren
 *
 * @param thread Pointer auf die Thread-Struktur
 *
 * @return true, wenn der Thread entsperrt wurde, sonst false
 */
bool pm_thread_unblock(pm_thread_t* thread)
{
    bool result = false;
    
    // Wenn der Thread gesperrt ist, muss garnicht weiter probiert werden
    if (locked(&thread->lock) == true) {
        return false;
    }

    lock(&thread->lock);

    // Der Thread wird nur entsperrt, wenn er auch wirklich blockiert ist
    if (thread->status == PM_STATUS_BLOCKED) {
        thread->status = PM_STATUS_READY;
        result = true;
    }

    unlock(&thread->lock);
    return result;
}

/**
 * Handler fuer das Loeschen des Threads registrieren
 */
void pm_thread_register_on_destroy(pm_thread_t* thread,
    pm_thread_destroy_handler handler, void* prv)
{
    struct on_destroy_info* info = malloc(sizeof(*info));

    info->handler = handler;
    info->prv = prv;

    list_push(thread->on_destroy, info);
}

/**
 * Gibt zurück, ob der Thread vom aktuellen Status in RUNNING wechseln darf.
 */
bool pm_thread_runnable(pm_thread_t* thread)
{
    return thread->status == PM_STATUS_READY
        || thread->status == PM_STATUS_WAIT_FOR_RPC
        || thread->status == PM_STATUS_WAIT_FOR_LIO;
}
