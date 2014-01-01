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
#include <syscallno.h>

#include "kernel.h"
#include "tasks.h"
#include "lock.h"

/// Liste mit allen verfuegbaren Threads
static list_t* threads_available;

// TODO: Fuers schedulen waere eine Double ended queue besser
/// Liste mit den geplanten Threads
static list_t* threads_scheduled;

/// Lock
static lock_t scheduler_lock;

/// Idle-Task
pm_thread_t* idle_task;

/**
 * Scheduler initialisieren
 */
void scheduler_init(void)
{
    // Die Listen initialisieren
    threads_available = list_create();
    threads_scheduled = list_create();
    
    // Sichergehen, dass der lock nicht gesperrt ist
    unlock(&scheduler_lock);
}

/**
 * Dem Scheduler einen neuen Thread hinzufuegen
 *
 * @param thread Thread
 */
void pm_scheduler_add(pm_thread_t* thread)
{
    lock(&scheduler_lock);
    list_push(threads_available, thread);
    unlock(&scheduler_lock);

    // Neu schedulen
    pm_scheduler_refresh();
}

/**
 * Einen Thread aus dem Scheduler entfernen
 * 
 * @param thread Thread
 */
void pm_scheduler_delete(pm_thread_t* thread)
{
    int i;
    pm_thread_t* p;

    lock(&scheduler_lock);

    // Die Liste mit den Threads durchsuchen
    for (i = 0; (p = list_get_element_at(threads_available, i)); i++) {
        if (p == thread) {
            list_remove(threads_available, i);
            break;
        }
    }

    // FIXME: Hier sollte noch waehrend der sperre neu geordnet werden
    unlock(&scheduler_lock);
    pm_scheduler_refresh();
}

/**
 * Die Liste im Scheduler aktualisieren
 */
void pm_scheduler_refresh()
{
    int i = 0;
    pm_thread_t* thread;
    
    lock(&scheduler_lock);
    
    // Liste mit dem geplanten Tasks leeren
    while (list_pop(threads_scheduled) != NULL);
    
    // Die Liste mit den Threads durchsuchen
    while ((thread = list_get_element_at(threads_available, i))) {
        // Der thread wird nur eingeplant, wenn er lauffaehig ist
        if ((locked(&thread->lock) == false)
            && (thread->status == PM_STATUS_READY)
            && (thread->process->status == PM_STATUS_READY))
        {
            list_push(threads_scheduled, thread);
            // kprintf("ok: 0x%x\n", thread->kernel_stack);
        } else {
            // kprintf("Nicht io: 0x%x %d  %d\n", thread->kernel_stack, thread->lock, thread->status);
        }
        i++;
    }

    unlock(&scheduler_lock);
}

/**
 * Einen Thread zum ausfuehren holen
 *
 * @return Einen Pointer auf eine Thread-Struktur oder NULL wenn kein Thread
 *          gefunden wurde.
 */
pm_thread_t* pm_scheduler_pop()
{
    pm_thread_t* thread = NULL;

    // Mehrere CPUs gleichzeitig im Scheduler sind sehr ungesund ;-)
    lock(&scheduler_lock);
    
    // Versuchen einen Thread aus der Liste zu nehmen
    thread = list_pop(threads_scheduled);

    // Falls das nicht geklappt hat, wird solange probiert, 
    if (thread == NULL) {
        // FIXME: Das ist so nicht wirklich geschickt, wenn der aufrufer dafür
        // sorgen muss, dass wir nicht hängen bleiben.
        unlock(&scheduler_lock);
        
        // Jetzt wird die Thread-Liste neu generiert
        pm_scheduler_refresh();

        // FIXME: s.o
        lock(&scheduler_lock);

        // Ein weiterer Versuch, einen Thread zu erhalten.
        // Wenn dann immer noch keiner bereit ist, kommt der Idle-Task zum Zug
        thread = list_pop(threads_scheduled);
        if (thread == NULL) {
            unlock(&scheduler_lock);
            idle_task->status = PM_STATUS_RUNNING;
            return idle_task;
        }
    }

    // Sobald der richtige Thread gefunden wurde, kann der Scheduler wieder
    // freigegeben werden.
    unlock(&scheduler_lock);
    
    // Thread-Struktur sperren und Status setzen
    lock(&thread->lock);
    if (thread->status != PM_STATUS_READY) {
        panic("Thread soll laufen, ist aber nicht bereit (Status %d)",
            thread->status);
    }
    thread->status = PM_STATUS_RUNNING;
    unlock(&thread->lock);
    
    return thread;
}

/**
 * Holt einen bestimmten Thread zum Ausfuehren
 */
void pm_scheduler_get(pm_thread_t* thread)
{
    int i;
    pm_thread_t* p;

    // Falls der Thread in der Liste der zu schedulenden Tasks ist, muessen wir
    // ihn rausloeschen. Ansonsten brauchen wir nichts zu machen.
    for (i = 0; (p = list_get_element_at(threads_scheduled, i)); i++) {
        if (p == thread) {
            list_remove(threads_scheduled, i);
            return;
        }
    }
}

/**
 * Einen ausgefuerten Thread wieder zurueck an den Scheduler geben
 *
 * @param thread Pointer auf eine Thread-Struktur
 */
void pm_scheduler_push(pm_thread_t* thread)
{
    // Den Task-Status wieder auf bereit setzen
    lock(&thread->lock);
    thread->status = PM_STATUS_READY;
    unlock(&thread->lock);
}

/**
 * Kontrolle vom aktuellen Kernelthread an einen anderen Thread abgeben
 */
void pm_scheduler_yield(void)
{
    asm volatile("int $0x30;" : : "a" (SYSCALL_PM_SLEEP));

    // Es kann passieren, dass zwei Tasks im Kernel hin- und heryielden und
    // auf irgendetwas warten, was von einem IRQ abhängt. Deshalb müssen wir
    // dem IRQ eine Chance geben, anzukommen.
    asm volatile("sti; nop; cli");
}

/**
 * Kontrolle vom aktuellen Kernelthread an einen anderen Thread abgeben
 *
 * @tid TID des Threads, an den die Kontrolle übergeben werden soll, oder 0
 * @status Thread-Status des alten Threads
 */
void pm_scheduler_kern_yield(tid_t tid, int status)
{
    asm volatile(
        "push %2;"
        "push %1;"
        "int $0x30;"
        "add $8, %%esp;"
        : : "a" (KERN_SYSCALL_PM_SLEEP), "r" (tid), "r" (status));

    // Es kann passieren, dass zwei Tasks im Kernel hin- und heryielden und
    // auf irgendetwas warten, was von einem IRQ abhängt. Deshalb müssen wir
    // dem IRQ eine Chance geben, anzukommen.
    asm volatile("sti; nop; cli");
}

/**
 * Macht den übergebenen Thread zum aktuellen Thread dieser CPU, wenn er
 * momentan lauffähig ist. Wenn nicht, bleibt der bisherige Thread aktiv.
 */
void pm_scheduler_try_switch(pm_thread_t* thread)
{
    cpu_t* cpu;

    if (thread->status != PM_STATUS_READY) {
        return;
    }

    cpu = cpu_get_current();
    if (cpu->thread != thread) {
        pm_scheduler_push(cpu->thread);
        pm_scheduler_get(thread);
        thread->status = PM_STATUS_RUNNING;
        cpu->thread = thread;
    }
}
