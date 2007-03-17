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
#include "stdlib.h"
#include "kmm.h"
#include "config.h"

 
#define USER_STACK_VIRT 0xfffff000
#define USER_STACK_SIZE (1024 - 4)
 



//struct task tasks[MAX_TASKS];

struct task* first_task;
int num_tasks;
int current_task_num;
qword timer_ticks = 0;

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
    /*current_task->esp = *esp;
    if(first_task == NULL) {
        panic("Taskliste leer!");
    }

    if(current_task == NULL) {
        current_task = first_task;
    }

    do {
        current_task = current_task->next_task;
        if(current_task == NULL)
            current_task = first_task;

    } while((current_task->blocked_by_pid) && (current_task->blocked_by_pid != current_task->pid));

    //kprintf("\nSwitch to %x", current_task->pid);
    *esp = current_task->esp;*/
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

    } while((current_task->blocked_by_pid) && (current_task->blocked_by_pid != current_task->pid));

    //kprintf("\nSwitch to %x", current_task->pid);
    *esp = current_task->esp;
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


/**
 * Generiert eine neue eindeutige Prozess-ID
 * @return neue Prozess-ID
 */
pid_t generate_pid()
{
    static pid_t next_pid = 1;
    return next_pid++;
}


/**
 * Erstellt einen neuen Task
 * @param entry Einsprungspunkt
 * @return 
 */
struct task * create_task(void* entry)
{
    struct task* new_task = malloc(sizeof(struct task));
    new_task->pid = generate_pid();
    
    //damit der Task seine ruhe hat, bis er die Blokade selbst aufhebt
    new_task->blocked_by_pid = new_task->pid;
    
    // haenge den Task an die Liste an
    /*if(first_task == NULL)
    {
        new_task->next_task = NULL;
        first_task = new_task;
    }
    else
    {*/
    new_task->next_task = first_task;
    first_task = new_task;
    //}
    //kprintf("new_task = 0x%x, new_task->next_task = 0x%x, first_task = 0x%x", (dword)new_task, (dword)new_task->next_task, (dword)first_task);
    // Speicher fuer die Stacks allokieren
    paddr_t phys_kernel_stack = phys_alloc_page();
    paddr_t phys_user_stack = phys_alloc_page();

    // Stacks mapen
    dword * kernel_stack = map_phys_addr(phys_kernel_stack, PAGE_SIZE);
    dword * user_stack = map_phys_addr(phys_user_stack, PAGE_SIZE);

    //kprintf("Neuer Task %d; KStack = 0x%x, UStack = 0x%x", num_tasks, kernel_stack, user_stack);

    // Der Stack wird von oben her beschrieben
    kernel_stack += 1024;
    phys_kernel_stack += 1024;

    // Page Directory anlegen
    paddr_t phys_pagedir = phys_alloc_page();
    //kprintf(", PD = 0x%x\n", new_task->cr3);
    
    page_directory_t pagedir = (page_directory_t) map_phys_addr(phys_pagedir, PAGE_SIZE);
    memset((void*) pagedir, 0, PAGE_SIZE);
    new_task->cr3 = pagedir;

    // Stack im PD des neuen Tasks mappen
    map_page(pagedir, (vaddr_t)USER_STACK_VIRT, phys_user_stack, PTE_W | PTE_P | PTE_U);
    new_task->user_stack_bottom = (vaddr_t) USER_STACK_VIRT;
    new_task->blocked_by_pid = 0;

    *(--kernel_stack) = 0x23; // ss
    *(--kernel_stack) = USER_STACK_VIRT + USER_STACK_SIZE - 0x4; // esp FIXME: das + 0x8 ist gehoert hier eigentlich nicht hin, das dient dazu dem Task startparameter mit zu geben und sollte flexibler geloest werden
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
    
    new_task->esp = (dword) kernel_stack;
    new_task->kernel_stack = (dword) kernel_stack;

    num_tasks++;
    
    return new_task;
}


void syscall_rpc(dword callee_pid, dword data_size, char* data);

void destroy_task(struct task* task_ptr)
{
    task_ptr->blocked_by_pid = -1;
    struct task* previous_task = NULL;
    struct task* next_task = NULL;
    // den Vorherigen Task in der liste heraussuchen
    if(task_ptr == first_task)
    {
        first_task = task_ptr->next_task;
    }
    else
    {
        next_task = first_task;
        while(next_task != task_ptr)
        {
            // Wenn der Task nicht gefunden wuzde verlasse die Funktion
            if(next_task == NULL)
                return;
            previous_task = next_task;
            next_task = next_task->next_task;
        }
        previous_task->next_task = task_ptr->next_task;
    }
    
    
    char* rpc_init = "        SERV_SHD    ";
    //rpc-by function name
    *((dword*)rpc_init) = 512;
    //pid setzen
    *((pid_t*)((dword) rpc_init + 8 + 8))  = task_ptr->pid;
    syscall_rpc(1, strlen(rpc_init), rpc_init);
    
    // Pagedir des tasks mapen
    page_directory_t page_dir = task_ptr->cr3;
    page_table_t page_table;
    
    int i;
    int n;
    
    // Alle pages von 0 - ende 3.GB 
    for(i=256; i < 1024; i++)
    {
        if(page_dir[i] & PTE_P)
        {
            page_table = map_phys_addr((paddr_t)(page_dir[i] & ~0xFFF), PAGE_TABLE_LENGTH);
            
            for(n=0; n < 1024; n++)
            {
                if(page_table[n] & PTE_P)
                {
                    phys_free_page((paddr_t) (page_table[n] & PAGE_MASK));
                }
            }
            
            free_phys_addr((vaddr_t)page_table, PAGE_TABLE_LENGTH);
        }
    }
    
    free_phys_addr((vaddr_t)page_dir, PAGE_DIRECTORY_LENGTH);
    free(task_ptr);
}


/**
 * Sucht einen Task anhand der PID und gibt einen Pointer darauf zurueck:
 * @param pid PID des gesuchten Tasks
 * @return Pointer auf den Task
 */
struct task * get_task(pid_t pid)
{
    struct task* curr_task = first_task;
    
    while(curr_task != NULL)
    {
        //kprintf("Current:%08x  next:%08x  next:%08x\n", (dword)(curr_task), (dword)(curr_task->next_task), (dword)(curr_task->next_task->next_task) );
        if(curr_task->pid == pid)
            return curr_task;
        
        curr_task = curr_task->next_task;
    }
    
    return NULL;
}
