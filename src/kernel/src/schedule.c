/*
 * Copyright (c) 2006-2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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

#include <lost/config.h>
#include "types.h"
#include "string.h"
#include "stdlib.h"
#include <stdint.h>

#include "kernel.h"
#include "intr.h"
#include "tasks.h"
#include "tss.h"
#include "kprintf.h"
#include "timer.h"

struct task* first_task = NULL;
struct task* current_task = NULL;
int num_tasks;
int current_task_num;

/** Einheit: Mikrosekunden */
uint64_t timer_ticks = 0;


void set_io_bitmap(void);

/**
 * Wird bei Kooperativem Multitasking benutzt zum handeln des Timers
 */
void do_nothing(void) {}


/**
 * Timer interrupt
 * @param irq IRQ-Nummer
 * @param esp Pointer auf Stackpointer
 */
void timer_irq(int irq, uint32_t* esp)
{
    timer_ticks += 1000000 / CONFIG_TIMER_HZ;

    //Ueberpruefen, ob der aktuelle Task alle seine Zeit verbraucht hat. Wenn
    // ja, wird ein neuer Task ausgewaehlt
    #ifndef CONFIG_COOPERATIVE_MULTITASKING
    if ((current_task == NULL) ||
        (--(current_task->schedule_ticks_left) >= 0))
    {
        schedule(esp);
    }
    #else
    if (current_task == NULL) {
        schedule(esp);
    }
    #endif


    // Ueberpruefen, ob ein Timer abgelaufen ist.
    // Dies muss nach dem schedule() kommen, da der Timer mit current_task ==
    // NULL nicht zurechtkommt.
    timer_notify(timer_ticks);
}


/**
 * Naechsten Task auswaehlen der ausgefuehrt werden soll
 * @param esp Pointer auf Stackpointer
 */
void schedule(uint32_t* esp)
{
    if(first_task == NULL) {
        panic("Taskliste leer!");
    }

    if (current_task != NULL) {
        //kprintf("\nSwitch from %d", current_task->pid);
        current_task->esp = *esp;
    } else {
        current_task = first_task;
    }

    struct task* old_task = current_task;

    do {
        current_task = current_task->next_task;
        
        if(current_task == NULL) {
            current_task = first_task;
        }

        if (current_task == old_task) {
            kprintf("%");
            break;
        }

    } while(((current_task->blocked_by_pid) 
        && (current_task->blocked_by_pid != current_task->pid))
        || (current_task->status != TS_RUNNING)
    );
    
    // Damit der Task die Zeit nicht beliebig hochspielen kann
    if (current_task->schedule_ticks_left > current_task->schedule_ticks_max) {
        current_task->schedule_ticks_left = current_task->schedule_ticks_max;
    }

    // Hier wird die CPU-Zeit berechnet, die der Task benutzen darf
    // Diese wird folgendermassen berechnet:
    // Benutzbare Ticks = Maximale Ticks + (Uebrige Ticks vom letzten Mal) / 2
    current_task->schedule_ticks_left = current_task->schedule_ticks_max +
        current_task->schedule_ticks_left / 2;

    //kprintf("\nSwitch to %d", current_task->pid);
    *esp = current_task->esp;

    // Wenn der Task auf einen Port zugreift, fliegt ein GPF.
    // Wir haben dann immer noch Zeit, die IO-Bitmap zu kopieren.
    tss.io_bit_map_offset = TSS_IO_BITMAP_NOT_LOADED;
}

void schedule_to_task(struct task* target_task, uint32_t* esp)
{
    if ((target_task != NULL) && ((!target_task->blocked_by_pid) || (target_task->blocked_by_pid == target_task->pid))) {
        current_task->esp = *esp;
        current_task = target_task;
        *esp = current_task->esp;
        
        tss.io_bit_map_offset = TSS_IO_BITMAP_NOT_LOADED;
    }
}

void set_io_bitmap()
{
    tss.io_bit_map_offset = TSS_IO_BITMAP_OFFSET;

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
    
    uint16_t reload_value = 1193182 / CONFIG_TIMER_HZ;
    // Timer umprogrammieren
    asm(
        "movb $0x34, %%al;"
        "outb %%al, $0x43;"
        "movw %0, %%ax;"
        "outb %%al, $0x40;"
        "movb %%ah, %%al;"
        "outb %%al, $0x40"
        : : "b" (reload_value) : "eax");
}
