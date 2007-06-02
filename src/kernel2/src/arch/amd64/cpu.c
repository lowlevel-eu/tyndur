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

#include "cpu.h"
#include "apic.h"
#include "kprintf.h"

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
    byte apic_id = apic_read(0x20) >> 24; 
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
void cpu_write_msr(dword msr, qword value)
{
    dword low = value & 0xFFFFFFFF;
    dword high = (value >> 32) & 0xFFFFFFFF;
    asm ("wrmsr" : : "a" (low), "d" (high),"c" (msr));
}

/**
 * Inhalt eines MSR auslesen
 *
 * @param msr Adresse des Registers
 *
 * @return Wert der im MSR gespeichert ist
 */
qword cpu_read_msr(dword msr)
{
    dword low, high;
    asm ("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
    return low | ((qword)high << 32);
}

/**
 * Die CPU-Register ausgeben
 *
 * @param machine_state
 */
void cpu_dump(machine_state_t* machine_state)
{
    printf("RAX=0x%016x  RBX=0x%016x  RCX=0x%016x\n", machine_state->rax, 
        machine_state->rbx, machine_state->rcx);
    printf("RDX=0x%016x  RSI=0x%016x  RDI=0x%016x\n", machine_state->rdx,
        machine_state->rsi, machine_state->rsi);
    printf("RSP=0x%016x  RBP=0x%016x  R08=0x%016x\n", machine_state->rsp,
        machine_state->rbp, machine_state->r[0]);
    printf("R09=0x%016x  R10=0x%016x  R11=0x%016x\n", machine_state->r[1],
        machine_state->r[2], machine_state->r[3]);
    printf("R12=0x%016x  R13=0x%016x  R14=0x%016x\n", machine_state->r[4],
        machine_state->r[5], machine_state->r[6]);
    printf("R15=0x%016x  RIP=0x%016x  RFL=0x%016x\n", machine_state->r[5],
        machine_state->rip, machine_state->rflags);

    printf("CS=0x%04x    DS=0x%04x  ES=0x%04x    FS=0x%04x  GS=0x%04x   "
        "SS=0x%04x\n", 
        machine_state->cs, machine_state->ds, machine_state->es,
        machine_state->fs, machine_state->gs, machine_state->ss);
}
