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
#include <lost/config.h>

#include "cpu.h"
#include "im.h"
#include "kprintf.h"
#include "tasks.h"
#include "debug.h"
#include "apic.h"
#include "kernel.h"
#include "syscall.h"

extern size_t cpu_count;

/** Einheit: Mikrosekunden */
uint64_t timer_ticks = 0;

static pm_process_t* intr_handling_task[IM_NUM_INTERRUPTS];
static uint32_t intr_to_send[IM_NUM_INTERRUPTS];

bool fastrpc_irq(pm_process_t* callee, size_t metadata_size, void* metadata,
    size_t data_size, void* data, uint8_t irq);

/**
 * Behandelt alle Interrupts. Wird aus den Interrupt-Stubs aufgerufen.
 * 
 * @return Pointer auf den neuen Stack-Frame
 */
interrupt_stack_frame_t* im_handler(interrupt_stack_frame_t* isf)
{
    // Pointer auf den aktuellen Thread holen    
    pm_thread_t* thread = cpu_get_current()->thread;
    pm_thread_t* old_thread = thread;
    thread->kernel_stack = (vaddr_t) isf;
    byte int_num = isf->interrupt_number;
    
    if (int_num < 20) {
        asm("cli");
        // Exception
        kprintf("\033[1;41m");
        kprintf("[CPU%d] Exception %d; Error Code = 0x%x\n", 
            cpu_get_current()->id, int_num, isf->error_code);

        kprintf("PID %d: %s\n",
            cpu_get_current()->thread->process->pid,
            cpu_get_current()->thread->process->cmdline);
        cpu_dump(isf);
        kprintf("\033[0;40m");

        // Ein Stacktrace duerfen wir nur ausgeben, wenn kein Pagefault wegen
        // dem Stack aufgetreten ist!
        uintptr_t cr2;
        asm("mov %%cr2, %0" : "=r" (cr2));
        if (!((cr2 < isf->esp + 800) && (cr2 >= isf->esp -0x20))) {
            stack_backtrace(isf->ebp, isf->eip);
        }

        // Hier werden alle Prozessoren zum halten gebracht
        if (cpu_count > 1) {
            apic_ipi_broadcast(0xF0, TRUE);
        }

        // Aber da das ohne APIC nicht so funktioniert, wird das noch
        // abgefangen.
        asm("cli; hlt");
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
            cpu_get_current()->thread = pm_scheduler_pop();
        } else {
            intr_to_send[int_num]++;
            im_disable_irq(int_num - IM_IRQ_BASE);
        }
    } else if (int_num == 0x30) {
        // Syscall
        syscall_arch(isf);
    } else if (int_num == 0xF0) {
        // Interrupt um Prozessor anzuhalten
        kprintf("\n[CPU%d] Angehalten", cpu_get_current()->id);
        asm("cli; hlt");
    } else {
        kprintf("Interrupt %d\n", int_num);
        if (int_num == 252) {
            apic_ipi_broadcast(0xF0, TRUE);
            asm("cli; hlt");
        }
    }

    thread = cpu_get_current()->thread;
    if (thread != old_thread) {
        cpu_get_current()->tss.ss0 = 2 << 3;
        cpu_get_current()->tss.esp0 =
            (uintptr_t) thread->kernel_stack
            + sizeof(interrupt_stack_frame_t);

        mmc_activate(&thread->process->context);
    }

    im_end_of_interrupt(int_num);
    return (interrupt_stack_frame_t*) thread->kernel_stack;
}

/**
 * Wird beim Loeschen eines Prozesses aufgerufen. Entfernt den Prozess aus der
 * Interrupthandlertabelle.
 */
static void on_process_destroy(pm_process_t* process, void* prv)
{
    uint32_t intr = (uint32_t) prv;

    if (intr >= IM_NUM_INTERRUPTS) {
        panic("im.c: on_process_destroy(): Korrupte Interruptnummer %d", intr);
    }

    if (intr_handling_task[intr] != process) {
        panic("im.c: on_process_destroy(): Prozess nicht fuer Interrupt %d "
            "zustaendig", intr);
    }

    intr_handling_task[intr] = NULL;
}

/**
 * Registriert einen Prozess als fuer einen Interrupt zustandig
 *
 * @return TRUE falls der Interrupt registriert werden konnte, FALSE falls fuer
 * ihn bereits einen Handler registriert war oder die Interruptnummer
 * ausserhalb des zulaessigen Bereichs liegt.
 *
 * TODO Mehrere Handler fuer einen Interrupt unterstuetzen
 */
bool im_add_handler(uint32_t intr, pm_process_t* handler)
{
    if ((intr >= IM_NUM_INTERRUPTS) || intr_handling_task[intr]) {
        return FALSE;
    }

    intr_handling_task[intr] = handler;
    pm_register_on_destroy(handler, on_process_destroy, (void*) intr);

    return TRUE;
}

/**
 * Ruft die Handler ausstehender Interrupts auf
 */
void im_send_interrupts(void)
{
    uint32_t intr;

    for (intr = 0; intr < IM_NUM_INTERRUPTS; intr++)
    {
        while (intr_to_send[intr] > 0) {
            if (intr_handling_task[intr] == NULL) {
                intr_to_send[intr] = 0;
                break;
            }

            // TODO schedule_to_task(intr_handling_task[intr], esp);

            if (!fastrpc_irq(intr_handling_task[intr], 0, 0,
                sizeof(intr), (char*) &intr, intr - IM_IRQ_BASE))
            {
                break;
            }
            intr_to_send[intr]--;
        }
    }
}
