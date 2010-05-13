/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
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
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "vmm.h"
#include "kmm.h"
#include "paging.h"
#include "kernel.h"
#include "shm.h"

#include "rpc.h"
#include "io.h"

#include "tasks.h"
#include "timer.h"
#include "kprintf.h"
#include "debug.h"

#define USER_STACK_VIRT 0xfffff000
#define USER_STACK_SIZE (1024 - 4)

extern uint64_t timer_ticks;
extern struct task* first_task;
extern int num_tasks;

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
 * Blockiert einen Task. Zu einem blockierten Task können keine RPC-Aufrufe
 * durchgeführt werden. Wenn der Task durch einen anderen Task blockiert ist,
 * wird er außerdem vom Scheduler nicht mehr aufgerufen, bis der Task 
 * entblockt wird.
 *
 * @param task Zu blockierender Task
 * @param blocked_by PID des blockierenden Tasks
 */
bool block_task(struct task* task, pid_t blocked_by)
{
    if(task->blocked_by_pid == blocked_by) {
        task->blocked_count++;
        return true;
    }

/*    while(task->blocked_by_pid)
    {
        kprintf("[block_task() => intr]\n");
        asm("int $0x20");
    }*/
    if (task->blocked_by_pid) {
        return false;
    }
    
    /*if (blocked_by == 10 || blocked_by == 11) {
        uint32_t ebp =((struct int_stack_frame*) current_task->esp)->ebp; 

        kprintf("block_task PID %d durch PID %d = %d an %08x\n", 
            task->pid, 
            blocked_by,
            current_task->pid,
            ebp >= 0x1000 ? *((uint32_t*) (ebp  + 4)) : 0
        );
    }*/

    task->blocked_by_pid = blocked_by;
    task->blocked_count = 1;

    return true;
}

/**
 * Entblockt einen blockierten Task. Dies ist nur möglich, wenn der
 * aufrufende Task dem blockierenden Task entspricht
 *
 * @param task Zu entblockender Task
 * @param blocked_by Aufrufender Task
 *
 * @return true, wenn der Task erfolgreich entblockt wurde, false
 * im Fehlerfall.
 */
bool unblock_task(struct task* task, pid_t blocked_by)
{
    if (task->blocked_by_pid == blocked_by) {
        if(--(task->blocked_count) == 0) {
            task->blocked_by_pid = 0;
        }
        return true;
    } else {
        kprintf("\033[41munblock_task nicht erlaubt.\n\n\n");
        return false;
    }
}


/**
 * Erstellt einen neuen Task
 *
 * @param entry Einsprungspunkt
 * @param args String der Parameter
 *
 * @return Neuer Task
 */
struct task * create_task(void* entry, const char* args, pid_t parent_pid)
{
    struct task* new_task = malloc(sizeof(struct task));
    new_task->pid = generate_pid();
    new_task->rpcs = list_create();
    new_task->io_bitmap = NULL;
    new_task->status = TS_RUNNING;
    
    new_task->shmids = list_create();
    new_task->shmaddresses = list_create();

    new_task->vm86 = false;
    new_task->vm86_info = NULL;
    
    new_task->memory_used = 0;
    // Wenn nicht Init erstellt werden soll, wir der Task mit der PID seines
    //  Elterntasks blockiert. Damit der Elterntask Zeit hat, die notwendigen
    //  Pages in den Adressraum des neuen Prozesses zu mappen.
    if (new_task->pid != 1) {
        new_task->blocked_count = 1;
        new_task->blocked_by_pid = parent_pid;
    } else {
        new_task->blocked_by_pid = 0;
        new_task->blocked_count = 0;
    }

    
    // Scheduling-Eigenschaften fuer den Task setzen
    new_task->schedule_ticks_left = 0;
    new_task->schedule_ticks_max = 50;

    // Haenge den Task an die Liste an
    // TODO Trennen der Task-Liste von der Scheduling-Liste
    new_task->next_task = first_task;
    first_task = new_task;
    if (parent_pid == 0) {
        new_task->parent_task = current_task;
    } else {
        new_task->parent_task = get_task(parent_pid);
    }
    
    // Kopieren der Kommandozeile
    new_task->cmdline = malloc(strlen(args) + 1);
    memcpy((void*) (new_task->cmdline), (void*)args, strlen(args) + 1);

    // Ein neues Page Directory anlegen. Das Page Directory bleibt
    // gemappt, solange der Task läuft.
    //
    // Der Kernelspeicher wird in allen Tasks gleich gemappt, der
    // Userspace bekommt erstmal einen komplett leeren Speicher.
    paddr_t phys_pagedir = phys_alloc_page();
    page_directory_t pagedir = (page_directory_t) map_phys_addr(phys_pagedir, PAGE_SIZE);

    memset((void*) pagedir, 0, PAGE_SIZE);
    memcpy((void*) pagedir, kernel_page_directory, 1024);
    pagedir[0xff] = (uint32_t) phys_pagedir | PTE_W | PTE_P;
    new_task->cr3 = pagedir;

    // Speicher fuer die Stacks allokieren
    // Stack im PD des neuen Tasks mappen
    paddr_t phys_kernel_stack = phys_alloc_page();
    paddr_t phys_user_stack = phys_alloc_page();

    map_page(pagedir, (vaddr_t)USER_STACK_VIRT, phys_user_stack, PTE_W | PTE_P | PTE_U);
    new_task->user_stack_bottom = (vaddr_t) USER_STACK_VIRT;

    // Den Kernelstack mappen und initialisieren
    // Der Stack wird von oben her beschrieben, daher 4K addieren 
    // (1024 * sizeof(uint32_t))
    uint32_t * kernel_stack = map_phys_addr(phys_kernel_stack, PAGE_SIZE);
    kernel_stack += 1024;

    *(--kernel_stack) = 0x23; // ss
    *(--kernel_stack) = USER_STACK_VIRT + USER_STACK_SIZE - 0x4; // esp FIXME: das + 0x8 ist gehoert hier eigentlich nicht hin, das dient dazu dem Task startparameter mit zu geben und sollte flexibler geloest werden
    *(--kernel_stack) = 0x0202; // eflags = interrupts aktiviert und iopl = 0
    *(--kernel_stack) = 0x1b; // cs
    *(--kernel_stack) = (uint32_t)entry; // eip

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
    
    new_task->esp = (uint32_t) kernel_stack;
    new_task->kernel_stack = (uint32_t) kernel_stack;

    num_tasks++;
    
    return new_task;
}

/**
 * Beendet einen Taks.
 *
 * @param task_ptr Zu beendender Task
 */
void destroy_task(struct task* task_ptr)
{
    task_ptr->blocked_by_pid = -1;

    struct task* previous_task = NULL;
    struct task* next_task = NULL;

    // Den vorherigen Task in der Liste heraussuchen
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

    // Alle Kindprozesse an init weitervererben
    for (next_task = first_task; next_task; next_task = next_task->next_task) {
        if (next_task->parent_task == task_ptr) {
            next_task->parent_task = get_task(1);
        }
    }

    // Interrupthandler ausschalten
    remove_intr_handling_task(task_ptr);
    
    
    // Melde das Beenden des Tasks per RPC an init
    char* rpc_init = "        SERV_SHD    ";
    *((uint32_t*)rpc_init) = 512;
    *((pid_t*)((uint32_t) rpc_init + 8 + 8))  = task_ptr->pid;
    
    fastrpc(get_task(1), 0, 0, strlen(rpc_init), rpc_init);
    
    // Timer freigeben
    timer_cancel_all(task_ptr);
    
    // Alle Ports und die IO-Bitmap des Tasks freigeben
    if (task_ptr->io_bitmap) {
        io_ports_release_all(task_ptr);
        free(task_ptr->io_bitmap);
    }
    
    //Shared Memory-Bereiche von Task lösen
    while (list_size(task_ptr->shmids) > 0) {
        uint32_t id = (uint32_t)list_get_element_at(task_ptr->shmids, list_size(task_ptr->shmids) - 1);
        detach_task_from_shm(task_ptr, id);
    }
    
    // Evtl. zusatzliche Infos von VM86-Task löschen
    if (task_ptr->vm86) {
        free(task_ptr->vm86_info);
    }

    // Kommandozeilenparameter freigeben
    if (task_ptr->cmdline) {
        free((char*) task_ptr->cmdline);
    }

    // Wenn der zu beendende Task kurz vorher einen RPC gemacht hat,
    // hat der aufgerufene Prozess den jetzt beendeten Task in
    // callee->rpcs und der Ruecksprung wurde im Desaster enden.
    rpc_destroy_task_backlinks(task_ptr);

    // Pagedir des tasks mappen, um den gesamten Speicher
    // freigeben zu können
    page_directory_t page_dir = task_ptr->cr3;
    page_table_t page_table;
    
    int i;
    int n;
    
    // Alle Pages von 1 GB - 4 GB
    //
    // FIXME Physisch freigeben nur, wenn die Seite tatsächlich speziell für
    // den Task reserviert wurde. Das ist beispielsweise bei Shared Memory oder
    // beim Zugriff auf physischen Speicher (Framebuffer) nicht der Fall.
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
   
    // Page Directory und den Task selbst freigeben
    // TODO Auch die physische Seite freigeben
    free_phys_addr((vaddr_t)page_dir, PAGE_DIRECTORY_LENGTH);
    free(task_ptr);
}


/**
 * Sucht einen Task anhand der PID und gibt einen Pointer darauf zurueck:
 * 
 * @param pid PID des gesuchten Tasks
 * @return Pointer auf den Task oder NULL, falls kein Task mit der passenden
 * PID gefunden wurde
 */
struct task * get_task(pid_t pid)
{
    struct task* curr_task = first_task;
    
    while(curr_task != NULL)
    {
        if(curr_task->pid == pid) {
            return curr_task;
        }
        
        curr_task = curr_task->next_task;
    }
    
    return NULL;
}

/**
 * Bricht den aktuell ausgeführten Task mit einer Fehlermeldung ab.
 */
void abort_task(char* format, ...)
{
    int * args = ((int*)&format) + 1;

    kprintf("\n\033[1;37m\033[41m" // weiss auf rot
        "Task %d beendet: %s\n", 
        current_task->pid,
        current_task->cmdline != NULL 
            ? current_task->cmdline 
            : "Unbekannter Task"
        );
    kaprintf(format, &args);
    kprintf("\n\033[0;37m\033[40m");
        
    if(debug_test_flag(DEBUG_FLAG_STACK_BACKTRACE))
    {
        struct int_stack_frame* isf = (struct int_stack_frame*)
            current_task->esp;
        stack_backtrace_ebp(isf->ebp, isf->eip);
    }

    destroy_task(current_task);
    
    current_task = NULL;
    asm("int $0x20");
}

