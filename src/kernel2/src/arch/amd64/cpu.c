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

#include "cpu.h"
#include "apic.h"
#include "kprintf.h"
#include "mm.h"

// Array mit Informationen zu allen Prozessoren (Sollte zu einer Liste gemacht
// werden.
cpu_t cpus[16];

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
    kprintf("RAX=0x%08x%08x  RBX=0x%08x%08x  RCX=0x%08x%08x\n", 
        (uint32_t) (machine_state->rax >> 32),
        (uint32_t) (machine_state->rax & 0xFFFFFFFF),
        (uint32_t) (machine_state->rbx >> 32),
        (uint32_t) (machine_state->rbx & 0xFFFFFFFF),
        (uint32_t) (machine_state->rcx >> 32),
        (uint32_t) (machine_state->rcx & 0xFFFFFFFF));
    kprintf("RDX=0x%08x%08x  RSI=0x%08x%08x  RDI=0x%08x%08x\n",
        (uint32_t) (machine_state->rdx >> 32),
        (uint32_t) (machine_state->rdx & 0xFFFFFFFF),
        (uint32_t) (machine_state->rsi >> 32),
        (uint32_t) (machine_state->rsi & 0xFFFFFFFF),
        (uint32_t) (machine_state->rdi >> 32),
        (uint32_t) (machine_state->rdi & 0xFFFFFFFF));
    kprintf("RSP=0x%08x%08x  RBP=0x%08x%08x  R08=0x%08x%08x\n",
        (uint32_t) (machine_state->rsp >> 32),
        (uint32_t) (machine_state->rsp & 0xFFFFFFFF),
        (uint32_t) (machine_state->rbp >> 32),
        (uint32_t) (machine_state->rbp & 0xFFFFFFFF),
        (uint32_t) (machine_state->r[0] >> 32),
        (uint32_t) (machine_state->r[0] & 0xFFFFFFFF));
    kprintf("R09=0x%08x%08x  R10=0x%08x%08x  R11=0x%08x%08x\n",
        (uint32_t) (machine_state->r[1] >> 32),
        (uint32_t) (machine_state->r[1] & 0xFFFFFFFF),
        (uint32_t) (machine_state->r[2] >> 32),
        (uint32_t) (machine_state->r[2] & 0xFFFFFFFF),
        (uint32_t) (machine_state->r[3] >> 32),
        (uint32_t) (machine_state->r[3] & 0xFFFFFFFF));
    kprintf("R12=0x%08x%08x  R13=0x%08x%08x  R14=0x%08x%08x\n",
        (uint32_t) (machine_state->r[4] >> 32),
        (uint32_t) (machine_state->r[4] & 0xFFFFFFFF),
        (uint32_t) (machine_state->r[5] >> 32),
        (uint32_t) (machine_state->r[5] & 0xFFFFFFFF),
        (uint32_t) (machine_state->r[6] >> 32),
        (uint32_t) (machine_state->r[6] & 0xFFFFFFFF));
    kprintf("R15=0x%08x%08x  RIP=0x%08x%08x  RFL=0x%08x%08x\n",
        (uint32_t) (machine_state->r[7] >> 32),
        (uint32_t) (machine_state->r[7] & 0xFFFFFFFF),
        (uint32_t) (machine_state->rip >> 32),
        (uint32_t) (machine_state->rip & 0xFFFFFFFF),
        (uint32_t) (machine_state->rflags >> 32),
        (uint32_t) (machine_state->rflags & 0xFFFFFFFF));

    kprintf("CS=0x%04x    DS=0x%04x  ES=0x%04x    FS=0x%04x  GS=0x%04x    "
        "SS=0x%04x\n", 
        machine_state->cs & 0xFFFF, machine_state->ds & 0xFFFF, 
        machine_state->es & 0xFFFF, machine_state->fs & 0xFFFF,
        machine_state->gs & 0xFFFF, machine_state->ss & 0xFFFF);
    
    uint64_t cr[5];
    asm("movq %%cr0, %0" : "=A" (cr[0]));
    asm("movq %%cr2, %0" : "=A" (cr[2]));
    asm("movq %%cr3, %0" : "=A" (cr[3]));
    asm("movq %%cr4, %0" : "=A" (cr[4]));
    asm("movq %%cr8, %0" : "=A" (cr[1]));
    
    kprintf("CR0=0x%016x  CR2=0x%016x  CR3=0x%016x\n", cr[0], cr[2], cr[3]);
    kprintf("CR4=0x%016x  CR8=0x%016x  EFR=0x%016x\n", cr[4], cr[1],
        cpu_read_msr(0xC0000080));
}

/**
 * Pointer auf den aktuellen Kontext von der CPU auslesen
 *
 * @return Kontext
 */
mmc_context_t cpu_get_context()
{
    uintptr_t context;
    asm("movq %%cr3, %0" : "=A" (context));
    return (mmc_context_t) (context  & (~0xFFF));
}

/**
 *
 * @param context Neuer Kontext
 */
void cpu_set_context(mmc_context_t context)
{
    asm("movq %0, %%cr3" : : "A" (context));
}

