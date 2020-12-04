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
#include <stdint.h>

#include "cpu.h"
#include "apic.h"
#include "kprintf.h"

// TODO
// Array mit Informationen zu allen Prozessoren (Sollte zu einer Liste gemacht
// werden, oder auch nicht... *g*).
cpu_t cpus[255];

// Anzahl der Verfuegbaren Prozessoren
size_t cpu_count = 0;

// CPU-ID des Bootstrap-Prozessors
cpu_id_t cpu_id_bootstrap;


/**
 * Neuen Prozessor hinzufuegen
 *
 * @param cpu CPU-Struktur
 */
cpu_id_t cpu_add_cpu(cpu_t cpu)
{
    cpu_id_t id = cpu_count;
    cpus[id] = cpu;
    cpus[id].id = id;
    cpu_count++;
    return id;
}

/**
 *
 */
cpu_t* cpu_get(cpu_id_t id)
{
    return &(cpus[id]);
}

/**
 *
 */
cpu_t* cpu_get_current()
{
    // Wenn kein APIC vorhanden ist, wird einfach die erste CPU benutzt
    if (cpu_count == 1) {
        return cpu_get(0);
    }

    uint8_t apic_id = apic_read(0x20) >> 24; 
    cpu_id_t i;

    for (i = 0; i < cpu_count; i++) {
        if (cpus[i].apic_id == apic_id) {
            return &(cpus[i]);
        }
    }

    return NULL;
}

/**
 * Inhalt eines MSR aendern
 * 
 * @param msr Adresse des Registers
 * @param value Neuer Wert
 */
void cpu_write_msr(uint32_t msr, uint64_t value)
{
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = (value >> 32) & 0xFFFFFFFF;
    asm ("wrmsr" : : "a" (low), "d" (high),"c" (msr));
}

/**
 * Inhalt eines MSR auslesen
 *
 * @param msr Adresse des Registers
 *
 * @return Wert der im MSR gespeichert ist
 */
uint64_t cpu_read_msr(uint32_t msr)
{
    uint32_t low, high;
    asm ("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
    return low | ((uint64_t)high << 32);
}


/**
 * Die CPU-Register ausgeben
 *
 * @param machine_state
 */
void cpu_dump(machine_state_t* machine_state)
{
    // TODO: ID der CPU
    kprintf("EAX=0x%016x  EBX=0x%016x  ECX=0x%016x\n", machine_state->eax,
        machine_state->ebx, machine_state->ecx);
    kprintf("EDX=0x%016x  ESI=0x%016x  EDI=0x%016x\n", machine_state->edx,
        machine_state->esi, machine_state->esi);
    kprintf("ESP=0x%016x  EBP=0x%016x  RIP=0x%016x\n", machine_state->esp,
        machine_state->ebp, machine_state->eip);

    uint32_t cr2, cr3;
    asm volatile("movl %%cr2, %0;"
                "movl %%cr3, %1;"
                : "=r"(cr2), "=r"(cr3));

    kprintf("EFL=0x%016x  CR2=0x%016x  CR3=0x%016x\n", machine_state->eflags,
        cr2, cr3);

    kprintf("CS=0x%04x    DS=0x%04x  ES=0x%04x    FS=0x%04x  GS=0x%04x   "
        "SS=0x%04x\n", 
        machine_state->cs, machine_state->ds, machine_state->es,
        machine_state->fs, machine_state->gs, machine_state->ss);

    if (machine_state->eflags & 0x20000) {
        struct vm86_isf* visf = (struct vm86_isf*) machine_state;
        kprintf("VM86:        DS=0x%04x  ES=0x%04x\n", visf->ds, visf->es);
    }
}

/**
 * Wird aufgerufen, nachdem zu einem neuen Task gewechselt wurde
 */
void cpu_prepare_current_task(void)
{
    pm_thread_t* thread = current_thread;
    interrupt_stack_frame_t* user_isf = thread->user_isf;

    cpu_get_current()->tss.ss0 = 2 << 3;
    if (user_isf->eflags & 0x20000) {
        cpu_get_current()->tss.esp0 =
            (uintptr_t) user_isf + sizeof(struct vm86_isf);
    } else {
        cpu_get_current()->tss.esp0 =
            (uintptr_t) user_isf + sizeof(interrupt_stack_frame_t);
    }

    cpu_get_current()->tss.io_bit_map_offset = TSS_IO_BITMAP_NOT_LOADED;
}

/*
 * Setzt die Register des ersten Threads des Prozesses, um ihm seinen
 * Prozessparameterblock bekannt zu machen.
 *
 * eax: ID des SHM
 * ebx: Pointer auf den gemappten SHM
 * ecx: Groesse des gemappten SHM
 */
int arch_init_ppb(pm_process_t* process, int shm_id, void* ptr, size_t size)
{
    pm_thread_t* thread = list_get_element_at(process->threads, 0);
    interrupt_stack_frame_t* user_isf = thread->user_isf;

    user_isf->eax = shm_id;
    user_isf->ebx = (uintptr_t) ptr;
    user_isf->ecx = size;

    return 0;
}
