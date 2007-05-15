/*
 * Copyright (c) 2006-2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Burkhard Weseloh.
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

#include "config.h"
#include "types.h"
#include "string.h"
#include "stdlib.h"

#include "kernel.h"
#include "intr.h"
#include "tasks.h"
#include "tss.h"
#include "kprintf.h"

struct task* first_task = NULL;
struct task* current_task = NULL;
int num_tasks;
int current_task_num;

qword timer_ticks = 0;


void set_io_bitmap();

/**
 * Wird bei Kooperativem Multitasking benutzt zum handeln des Timers
 */
void do_nothing() {}


/**
 * Timer interrupt
 * @param irq IRQ-Nummer
 * @param esp Pointer auf Stackpointer
 */
void timer_irq(int irq, dword* esp)
{
    timer_ticks += 10000;
    #ifdef CONFIG_COOPERATIVE_MULTITASKING
        release_irq(0);
        request_irq(0, do_nothing);
    #endif
    
    schedule(esp);
}

/**
 * Naechsten Task auswaehlen der ausgefuehrt werden soll
 * @param esp Pointer auf Stackpointer
 */
void schedule(dword* esp)
{
    if(first_task == NULL) {
        panic("Taskliste leer!");
    }

    if (current_task != NULL) {        
        current_task->esp = *esp;
    } else {
        current_task = first_task;
    }

    do {
        current_task = current_task->next_task;
        if(current_task == NULL)
            current_task = first_task;

    } while(((current_task->blocked_by_pid) 
        && (current_task->blocked_by_pid != current_task->pid))
        || (current_task->status != TS_RUNNING)
    );

    //kprintf("\nSwitch to %x", current_task->pid);
    *esp = current_task->esp;
    
    set_io_bitmap();
}

void schedule_to_task(struct task* target_task, dword* esp) 
{
    if ((target_task != NULL) && ((!target_task->blocked_by_pid) || (target_task->blocked_by_pid == target_task->pid))) {
        current_task->esp = *esp;
        current_task = target_task;
        *esp = current_task->esp;
        
        set_io_bitmap();
    }
}

void set_io_bitmap()
{
    if (current_task->io_bitmap) {
        memcpy(tss.io_bit_map, current_task->io_bitmap, IO_BITMAP_LENGTH / 8);
    } else {
        memset(tss.io_bit_map, 0xff, IO_BITMAP_LENGTH / 8);
    }
}

/**
 * Initialisiert das Multitasking
 */
void init_scheduler(void)
{
    request_irq(0, timer_irq);
    
    num_tasks = 0;
    current_task = NULL;
    first_task = NULL;
    
    //Timer auf 100Hz umstellen
    asm(
        "movb $0x36, %%al;"
        "outb %%al, $0x43;"
        "movb $0x9B, %%al;"
        "outb %%al, $0x40;"
        "movb $0x2E, %%al;"
        "outb %%al, $0x40"
        : : : "eax");
}
