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
#include <config.h>
#include <stdlib.h>
#include <collections.h>

#include "tasks.h"
#include "mm.h"
#include "lock.h"
#include "cpu.h"

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
    pm_thread_t* thread = malloc(sizeof(pm_thread_t));
    if (thread == NULL) {
        return NULL;
    }
    
    // Ein Pointer auf den Prozess im Thread hinterlegen
    thread->process = process;
    
    // Der Lock wird gesperrt initialisiert
    thread->lock = 1;

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

    //  Wir wollen alle Register schoen mit 0 initialisiert haben
    memset(isf, 0, sizeof(interrupt_stack_frame_t));

    #if CONFIG_ARCH == ARCH_I386
        isf->esp = (dword) process->next_stack;
        isf->eip = (dword) entry;
        
        // Flags: Interrupts aktivieren, und PL fuer IO-Operationen auf 3
        // setzen.
        isf->eflags = 0x0202;
        
        isf->cs = 0x8;
        isf->ds = 0x10;
        isf->es = 0x10;
        isf->fs = 0x10;
        isf->gs = 0x10;
    #else
        isf->rsp = (qword) process->next_stack;
        isf->rip = (qword) entry;
    #endif
    
    process->next_stack = (vaddr_t) ((uintptr_t) process->next_stack - 
        USER_STACK_SIZE);
    
    // Page fuer den Userspace-Stack allozieren
    paddr_t stack_phys = pmm_alloc(1);
    mmc_map(&process->context, (vaddr_t) (USER_STACK_START - USER_STACK_SIZE + 
        1), stack_phys, MM_FLAGS_USER_DATA, 1);
    
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
    // Zuerst muss der Thread angehalten werden
    while (pm_thread_block(thread) == FALSE);
    
    // Thread sperren
    lock(&thread->lock);

    // Beim Scheduler abmelden
    pm_scheduler_delete(thread);

    // TODO: Den Userspacestack auch freigeben

    // Kernelstack freigeben
    mmc_unmap(&thread->process->context, thread->kernel_stack_bottom,
        NUM_PAGES(thread->kernel_stack_size));
    
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
 * Thread blockieren
 *
 * @param thread Pointer auf die Thread-Struktur
 *
 * @return TRUE, wenn der Thread gesperrt wurde, sonst FALSE
 */
bool pm_thread_block(pm_thread_t* thread)
{
    // TODO: In SMP-Systemen sollte hier ein IPI kommen, falls der Thread auf
    // einer anderen CPU am laufen ist, der sie dazu zwingt, den Thread
    // abzugeben.
    bool result = FALSE;
    
    // Wenn der Thread gesperrt ist, muss garnicht weiter probiert werden
    if (locked(&thread->lock) == TRUE) {
        return FALSE;
    }

    lock(&thread->lock);

    if (thread->status == PM_STATUS_READY) {
        thread->status = PM_STATUS_BLOCKED;
        result = TRUE;
    }
    
    unlock(&thread->lock);

    return result;
}

/**
 * Thread entsperren
 *
 * @param thread Pointer auf die Thread-Struktur
 *
 * @return TRUE, wenn der Thread entsperrt wurde, sonst FALSE
 */
bool pm_thread_unblock(pm_thread_t* thread)
{
    bool result = FALSE;
    
    // Wenn der Thread gesperrt ist, muss garnicht weiter probiert werden
    if (locked(&thread->lock) == TRUE) {
        return FALSE;
    }

    lock(&thread->lock);

    // Der Thread wird nur entsperrt, wenn er auch wirklich blockiert ist
    if (thread->status == PM_STATUS_BLOCKED) {
        lock(&thread->lock);
        unlock(&thread->lock);
        return TRUE;
    }

    unlock(&thread->lock);
    return result;
}

