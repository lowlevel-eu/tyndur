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

#include <stdint.h>
#include <types.h>
#include "mm.h"

typedef uint8_t cpu_id_t;

typedef struct {
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t r[8];

    uint64_t ds;
    uint64_t es;

    uint64_t rax;

    uint64_t interrupt_number;
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;

    uint64_t fs, gs;
} __attribute__((packed)) interrupt_stack_frame_t;

/**
 * Überprüft ob es sich um ein Stackframe aus dem Userspace handelt.
 *
 * @return true wenn es ein Userspace Stackframe ist
 */
static inline bool isf_is_userspace(interrupt_stack_frame_t* isf)
{
    return (isf->cs & 0x3);
}

typedef interrupt_stack_frame_t machine_state_t;

/// Task state segment
typedef struct {
	uint32_t : 32;
	uint64_t sp0, sp1, sp2;
	uint64_t : 64;
	uint64_t ist[7];
	uint64_t : 64;
	uint16_t : 16;
	uint16_t io_bit_map_offset;
} __attribute__((packed)) cpu_tss_t;

// Dieser Wert muss ausserhalb des in der GDT definierten Limits
// fuer das TSS liegen
#define TSS_IO_BITMAP_NOT_LOADED UINT16_MAX

typedef struct {
    cpu_id_t id;
    uint8_t apic_id;
    bool bootstrap;
    cpu_tss_t tss;
    pm_thread_t* thread;
    mmc_context_t* mm_context;
} cpu_t;

extern cpu_id_t cpu_id_bootstrap;

cpu_id_t cpu_add_cpu(cpu_t cpu);
cpu_t* cpu_get(cpu_id_t id);

void cpu_write_msr(uint32_t msr, uint64_t value);
uint64_t cpu_read_msr(uint32_t reg);

mmc_context_t cpu_get_context();

/*
 * Aktiviert die I/O-Bitmap fuer den Prozess
 */
void io_activate_bitmap(pm_process_t* task);

/**
 * Hält die CPU an
 */
static inline void cpu_halt(void)
{
    asm volatile ("cli; hlt");
}

#endif //ifndef _CPU_ARCH_H_
