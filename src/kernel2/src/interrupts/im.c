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

#include <stdbool.h>
#include <lock.h>
#include <types.h>
#include <lost/config.h>

#include "cpu.h"
#include "im.h"
#include "kprintf.h"
#include "tasks.h"
#include "debug.h"
#include "apic.h"
#include "kernel.h"
#include "syscall.h"
#include "timer.h"

extern size_t cpu_count;

/** Einheit: Mikrosekunden */
uint64_t timer_ticks = 0;

#define MAX_INTERRUPTS 4

// Wenn mehrere Tasks für einen Interrupt registriert sind, dann werden sie im
// Array so angeordnet, dass nur die ersten Einträge gefüllt sind
static pm_process_t* intr_handling_task[IM_NUM_INTERRUPTS][MAX_INTERRUPTS];
static uint32_t intr_to_send[IM_NUM_INTERRUPTS][MAX_INTERRUPTS];

// Anzahl von Tasks pro Interrupt
static int intr_num_tasks[IM_NUM_INTERRUPTS];

// Gibt an, ob überhaupt irgendein Interrupt für die jeweilige Nummer
// auszuliefern ist (ist also false, wenn alle intr_to_send-Einträge für den
// Interrupt 0 sind, sonst true).
static bool any_intr[IM_NUM_INTERRUPTS];

// Anzahl von Tasks, die sich gerade um einen bestimmten IRQ kümmern (zu denen
// also ein fastrpc_irq ausgeführt wurde und noch nicht beendet ist)
static uint32_t intr_active_tasks[IM_NUM_INTERRUPTS];
// Ein Lock für dieses Array
static lock_t intr_active_tasks_lock = LOCK_UNLOCKED;

void im_send_interrupts(void);
bool fastrpc_irq(pm_process_t* callee, size_t metadata_size, void* metadata,
    size_t data_size, void* data, uint8_t irq);

void handle_exception(interrupt_stack_frame_t* isf, uint8_t int_num);

/**
 * Behandelt alle Interrupts. Wird aus den Interrupt-Stubs aufgerufen.
 * 
 * @return Pointer auf den neuen Stack-Frame
 */
interrupt_stack_frame_t* im_handler(interrupt_stack_frame_t* isf)
{
    // Pointer auf den aktuellen Thread holen    
    pm_thread_t* thread = current_thread;
    pm_thread_t* old_thread = thread;
    uint8_t int_num = isf->interrupt_number;
    interrupt_stack_frame_t* new_isf = isf;
    uintptr_t isf_addr = (uintptr_t) isf;
#ifndef CONFIG_RELEASE_VERSION
#ifdef CONFIG_ARCH_I386
    uint32_t eax = isf->eax;
#else
    uint32_t eax = 0;
#endif
#endif

    if ((isf_addr < (uintptr_t) thread->kernel_stack_bottom) ||
        (isf_addr >=
        ((uintptr_t) thread->kernel_stack_bottom + thread->kernel_stack_size)))
    {
        panic("Stackframe von Interrupt liegt ausserhalb des Kernelstacks des "
            "aktuellen Threads. (pm_scheduler_yield() nach Thread-Wechsel?)");
    }

    // Wenn der Int aus dem Userspace kommt, legen wir den zugehoerigen ISF ab
    if (isf_is_userspace(isf)) {
        thread->user_isf = isf;
    }

    if (int_num < 20) {
        handle_exception(isf, int_num);
    } else if ((int_num >= 20) && (int_num < 32)){
        // Reservierter Interrupt
        kprintf("Reserved Interrupt %d\n", int_num);
    } else if ((int_num >= IM_IRQ_BASE) && (int_num
        < (IM_IRQ_BASE + 16)))
    {
        // Beim Timer-IRQ wird geschedulet
        if (int_num == 0x20) {
            timer_ticks += 1000000 / CONFIG_TIMER_HZ;

            // Den aktuellen Thread an den Scheduler zurueckgeben
            pm_scheduler_push(thread);

            // Einen neuen Thread holen.
            current_thread = pm_scheduler_pop();

            // Timer ausloesen, wenn noetig
            timer_notify(timer_ticks);
        } else {
            int i;
            for (i = 0; i < intr_num_tasks[int_num]; i++) {
                intr_to_send[int_num][i]++;
            }
            any_intr[int_num] = true;
            im_disable_irq(int_num - IM_IRQ_BASE);
        }
    } else if (int_num == 0x30) {
        // Syscall
        syscall_arch(isf);
    } else if (int_num == 0xF0) {
        // Interrupt um Prozessor anzuhalten
        kprintf("\n[CPU%d] Angehalten", cpu_get_current()->id);
        cpu_halt();
    } else {
        kprintf("Interrupt %d\n", int_num);
        if (int_num == 252) {
            apic_ipi_broadcast(0xF0, true);
            cpu_halt();
        }
    }

    im_send_interrupts();

    thread = current_thread;

    if (thread != old_thread) {

#ifndef CONFIG_RELEASE_VERSION
        if (thread->status != PM_STATUS_RUNNING) {
            panic("Aktiver Task hat nicht PM_STATUS_RUNNING: "
                "PID %d, int %x, eax = %d\n",
                thread->process->pid, int_num, eax);
        }
#endif

        /* FIXME Was bei einem exit-Syscall? */
        old_thread->kernel_stack = isf;
        new_isf = im_prepare_current_thread();
    }

    im_end_of_interrupt(int_num);
    return new_isf;
}

/**
 * Aktuellen Thread zum laufen vorbereiten nach einem Thread-Wechsel. Konkret
 * TSS befuellen, in den richtigen Speicherkontext wechseln, und den Interrupt-
 * Stack-Frame raussuchen.
 */
interrupt_stack_frame_t* im_prepare_current_thread(void)
{
    pm_thread_t* thread = current_thread;

    mmc_activate(&thread->process->context);
    cpu_prepare_current_task();

    return thread->kernel_stack;
}

/**
 * Wird beim Loeschen eines Prozesses aufgerufen. Entfernt den Prozess aus der
 * Interrupthandlertabelle.
 */
static void on_process_destroy(pm_process_t* process, void* prv)
{
    uint32_t intr = (uintptr_t) prv;

    if (intr >= IM_NUM_INTERRUPTS) {
        panic("im.c: on_process_destroy(): Korrupte Interruptnummer %d", intr);
    }

    int i, j;
    for (i = 0; i < intr_num_tasks[intr]; i++) {
        if (intr_handling_task[intr][i] == process) {
            // Nach vorn schieben, sodass immer nur die vorderen Plätze des
            // Arrays belegt sind
            int last = --intr_num_tasks[intr];
            for (j = i + 1; j <= last; j++) {
                intr_handling_task[intr][j - 1] = intr_handling_task[intr][j];
            }
            intr_handling_task[intr][last] = NULL;

            return;
        }
    }

    panic("im.c: on_process_destroy(): Prozess nicht fuer Interrupt %d "
        "zustaendig", intr);
}

/**
 * Registriert einen Prozess als fuer einen Interrupt zustandig
 *
 * @return true falls der Interrupt registriert werden konnte, false falls fuer
 * ihn bereits einen Handler registriert war oder die Interruptnummer
 * ausserhalb des zulaessigen Bereichs liegt.
 *
 * TODO Mehrere Handler fuer einen Interrupt unterstuetzen
 */
bool im_add_handler(uint32_t intr, pm_process_t* handler)
{
    if (intr >= IM_NUM_INTERRUPTS) {
        return false;
    }

    if (intr_num_tasks[intr] >= MAX_INTERRUPTS) {
        // Kein Platz für diesen IRQ mehr übrig
        return false;
    }

    intr_handling_task[intr][intr_num_tasks[intr]++] = handler;
    pm_register_on_destroy(handler, on_process_destroy,
        (void*)(uintptr_t) intr);

    // Moeglicherweise ist der IRQ frueher maskiert worden, weil es keinen
    // Treiber dafuer gab. Spaetestens jetzt brauchen wir ihn aber wieder.
    if ((intr >= IM_IRQ_BASE) && (intr < (IM_IRQ_BASE + 16))) {
        im_enable_irq(intr - IM_IRQ_BASE);
    }

    return true;
}

/**
 * Ruft die Handler ausstehender Interrupts auf
 */
void im_send_interrupts(void)
{
    uint32_t intr;
    pm_thread_t* old_thread = current_thread;
    pm_thread_t* new_thread = old_thread;

    for (intr = 0; intr < IM_NUM_INTERRUPTS; intr++)
    {
        if (!any_intr[intr]) {
            continue;
        }

        any_intr[intr] = false;

        int i;
        for (i = 0; i < intr_num_tasks[intr]; i++) {
            while (intr_to_send[intr][i] > 0) {
                // Dieses Wechseln des Tasks ist wichtig: IRQs werden nur
                // angenommen, wenn der Task sie sich selbst schickt
                current_thread = new_thread =
                    list_get_element_at(intr_handling_task[intr][i]->threads, 0);

                if (!fastrpc_irq(intr_handling_task[intr][i], 0, 0,
                    sizeof(intr), (char*) &intr, intr - IM_IRQ_BASE))
                {
                    break;
                }
                intr_to_send[intr][i]--;
            }

            if (intr_to_send[intr][i]) {
                any_intr[intr] = true;
            }
        }
    }

    current_thread = old_thread;
    if (new_thread!= old_thread) {
        pm_scheduler_try_switch(new_thread);
    }
}

void im_irq_handlers_increment(uint8_t irq)
{
    lock(&intr_active_tasks_lock);
    intr_active_tasks[irq]++;
    unlock(&intr_active_tasks_lock);
}

void im_irq_handlers_decrement(uint8_t irq)
{
    lock(&intr_active_tasks_lock);

    if (--intr_active_tasks[irq] <= 0) {
        intr_active_tasks[irq] = 0;
        im_enable_irq(irq);
    }

    unlock(&intr_active_tasks_lock);
}
