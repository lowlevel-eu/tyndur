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
#ifndef _CPU_H_
#define _CPU_H_

#include <types.h>

#include "tasks.h"

typedef byte cpu_id_t;

typedef struct {
    dword ebx;
    dword ecx;
    dword edx;
    dword esi;
    dword edi;
    dword ebp;
    
    dword ds;
    dword es;
    dword fs;
    dword gs;

    dword eax;

    dword interrupt_number;
    dword error_code;
    dword eip;
    dword cs;
    dword eflags;
    dword esp;
    dword ss;
} __attribute__((packed)) interrupt_stack_frame_t;


#define CPU_IO_BITMAP_LENGTH 0xffff

/// Task state segment
typedef struct {
	dword backlink;
	dword esp0;
	dword ss0;
	dword esp1;
	dword ss1;
	dword esp2;
	dword ss2;
	dword cr3;
	dword eip;
	dword eflags;
	dword eax;
	dword ecx;
	dword edx;
	dword ebx;
	dword esp;
	dword ebp;
	dword esi;
	dword edi;
	dword es;
	dword cs;
	dword ss;
	dword ds;
	dword fs;
	dword gs;
	dword ldt;
	word trace_trap;
	word io_bit_map_offset;
    byte io_bit_map[CPU_IO_BITMAP_LENGTH / 8];
    byte io_bit_map_end;
} __attribute__((packed)) cpu_tss_t;

typedef struct {
    cpu_id_t id;
    byte apic_id;
    bool bootstrap;
    cpu_tss_t tss;
    pm_thread_t* thread;
    vaddr_t apic_base;
} cpu_t;

typedef interrupt_stack_frame_t machine_state_t;

extern cpu_t cpus[];
extern size_t cpu_count;
extern cpu_id_t cpu_id_bootstrap;

cpu_id_t cpu_add_cpu(cpu_t cpu);
cpu_t* cpu_get(cpu_id_t id);
void cpu_write_msr(dword msr, qword value);
qword cpu_read_msr(dword reg);
cpu_t* cpu_get_current();
void cpu_dump(machine_state_t* machine_state);

#endif //ifndef _CPU_H_

