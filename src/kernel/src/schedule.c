/*
 * Copyright (c) 2006 The LOST Project. All rights reserved.
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

#include "types.h"
#include "kernel.h"
#include "intr.h"
#include "paging.h"
#include "kprintf.h"
#include "string.h"
#include "tasks.h"
  
 
#define USER_STACK_VIRT 0xfffff000
#define USER_STACK_SIZE (1024 - 4)
 

/* TODO: rewrite */

struct task tasks[MAX_TASKS];
int num_tasks;
int current_task_num;

static void timer_irq(int irq, dword* esp)
{
    current_task->esp = *esp;

    do {
        current_task_num++;
    } while(tasks[current_task_num].blocked_by_pid);

    if(current_task_num >= num_tasks)
    {
        current_task_num = 0;
    }

    current_task = &tasks[current_task_num];
    *esp = current_task->esp;
}

void init_scheduler(void)
{
    request_irq(0, timer_irq);

    num_tasks = 0;

    current_task_num = -1;
    tasks[current_task_num].type = 0;
    current_task = &tasks[current_task_num];
}

struct task * create_task(void * entry)
{
    dword * kernel_stack = ((dword*)phys_alloc_page());
    dword * user_stack = ((dword*)phys_alloc_page());

    map_page(kernel_page_directory, kernel_stack, kernel_stack, PTE_P | PTE_W | PTE_U);
    map_page(kernel_page_directory, user_stack, user_stack, PTE_P | PTE_W | PTE_U);

    kprintf("Neuer Task %d; KStack = 0x%x, UStack = 0x%x", num_tasks, kernel_stack, user_stack);

    kernel_stack += 1024;
//    user_stack += 1024;

    // Page Directory anlegen
    tasks[num_tasks].cr3 = (dword)phys_alloc_page();
    kprintf(", PD = 0x%x\n", tasks[num_tasks].cr3);
    kernel_identity_map((paddr_t)(tasks[num_tasks].cr3), PAGE_SIZE);
    memset((void*)tasks[num_tasks].cr3, 0, PAGE_SIZE);
    
    // Stack mappen
    map_page((page_directory_t)tasks[num_tasks].cr3, (vaddr_t)USER_STACK_VIRT, (paddr_t)user_stack, PTE_W | PTE_P | PTE_U);

    tasks[num_tasks].kernel_stack = (dword)kernel_stack;
    tasks[num_tasks].blocked_by_pid = 0;

    *(--kernel_stack) = 0x23; // ss
    *(--kernel_stack) = USER_STACK_VIRT + USER_STACK_SIZE; // esp
    *(--kernel_stack) = 0x3202; // eflags = interrupts aktiviert und iopl = 3
    *(--kernel_stack) = 0x1b; // cs
    *(--kernel_stack) = (dword)entry; // eip

    *(--kernel_stack) = 0; // interrupt nummer
    *(--kernel_stack) = 0; // error code

    // general purpose registers
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;

    // segment registers
    *(--kernel_stack) = 0x23;
    *(--kernel_stack) = 0x23;
    *(--kernel_stack) = 0x23;
    *(--kernel_stack) = 0x23;

    tasks[num_tasks].esp = tasks[num_tasks].kernel_stack = (dword)kernel_stack;
    num_tasks++;

    return &tasks[num_tasks - 1];
}

struct task * get_task(pid_t pid)
{
    return &tasks[pid - 1];
}

pid_t get_pid(struct task * task)
{
    int i;

    for (i = 0; i < MAX_TASKS; i++) {
        if (&tasks[i] == task) {
            return i + 1;
        }
    }

    return 0;
}
