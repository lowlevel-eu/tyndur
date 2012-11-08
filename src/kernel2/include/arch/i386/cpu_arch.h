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
#ifndef _CPU_ARCH_H_
#define _CPU_ARCH_H_

#include <types.h>
#include <stdint.h>
#include <stdbool.h>

#include "tasks.h"

typedef uint8_t cpu_id_t;

typedef struct {
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t gs;

    uint32_t eax;

    uint32_t interrupt_number;
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
} __attribute__((packed)) interrupt_stack_frame_t;

/**
 * Überprüft ob es sich um ein Stackframe aus dem Userspace handelt.
 *
 * @return true wenn es ein Userspace Stackframe ist
 */
static inline bool isf_is_userspace(interrupt_stack_frame_t* isf)
{
    return (isf->cs & 0x3) || (isf->eflags & 0x20000);
}

/** Der erweiterte Interruptstackframe fuer VM86-Tasks */
struct vm86_isf {
    interrupt_stack_frame_t isf;

    uint32_t    es;
    uint32_t    ds;
    uint32_t    fs;
    uint32_t    gs;
} __attribute__((packed));



#define CPU_IO_BITMAP_LENGTH 0xffff

// Dieser Wert muss ausserhalb des in der GDT definierten Limits
// fuer das TSS liegen
#define TSS_IO_BITMAP_NOT_LOADED (sizeof(cpu_tss_t) + 0x100)
#define TSS_IO_BITMAP_OFFSET offsetof(cpu_tss_t, io_bit_map)

/// Task state segment
typedef struct {
	uint32_t backlink;
	uint32_t esp0;
	uint32_t ss0;
	uint32_t esp1;
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t trace_trap;
	uint16_t io_bit_map_offset;
    uint8_t io_bit_map[CPU_IO_BITMAP_LENGTH / 8];
    uint8_t io_bit_map_end;
} __attribute__((packed)) cpu_tss_t;

typedef struct {
    cpu_id_t id;
    uint8_t apic_id;
    bool bootstrap;
    cpu_tss_t tss;
    uint16_t tss_selector;
    pm_thread_t* thread;
    vaddr_t apic_base;
    mmc_context_t* mm_context;
} cpu_t;

typedef interrupt_stack_frame_t machine_state_t;

extern cpu_id_t cpu_id_bootstrap;

cpu_id_t cpu_add_cpu(cpu_t cpu);
cpu_t* cpu_get(cpu_id_t id);
void cpu_write_msr(uint32_t msr, uint64_t value);
uint64_t cpu_read_msr(uint32_t reg);

/**
 * Reserviert einen Bereich von IO-Ports für einen Task.
 * Es werden entweder alle angeforderten oder im Fehlerfall gar keine Ports
 * reserviert. Eine teilweise Reservierung tritt nicht auf.
 *
 * @param task Task, für den die Ports reserviert werden sollen
 * @param port Nummer des niedrigsten zu reservierenden Ports
 * @param length Anzahl der Ports, die zu reservieren sind
 */
bool io_ports_request(pm_process_t* task, uint32_t port, uint32_t length);

/**
 * Gibt einen Bereich von Ports frei.
 *
 * @param task Task, der die Freigabe anfordert
 * @param port Niedrigster freizugebender Port
 * @param length Anzahl der freizugebenden Ports
 *
 * @return true, wenn alle Ports freigegeben werden konnten. Wenn ein Port
 * nicht freigegeben werden konnte (war nicht für den Task reserviert),
 * wird false zurückgegeben, die Bearbeitung allerdings nicht abgebrochen,
 * sondern die weiteren Ports versucht freizugeben.
 */
bool io_ports_release(pm_process_t* task, uint32_t port, uint32_t length);

/**
 * Gibt alle von einem Task reservierten Ports frei
 *
 * @param task Task, dessen Ports freigegeben werden sollen
 */
void io_ports_release_all(pm_process_t* task);

/*
 * Aktiviert die I/O-Bitmap fuer den Prozess
 */
void io_activate_bitmap(pm_process_t* task);

void io_ports_check(pm_process_t* task);

/**
 * Hält die CPU an.
 */
static inline void cpu_halt(void)
{
    asm volatile ("cli; hlt");
}

#endif //ifndef _CPU_ARCH_H_
