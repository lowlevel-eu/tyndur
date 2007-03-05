
/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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

#include "syscall.h"
#include "kprintf.h"
#include "vmm.h"
#include "paging.h"
#include "kernel.h"
#include "string.h"
#include "kmm.h"
#include "tasks.h"
#include "stdlib.h"
#include "intr.h"

extern void timer_irq(int irq, dword* esp);
extern unsigned long timer_ticks;

void abort_task(char* format, ...);
void block_task(struct task* task, pid_t blocked_by);
bool unblock_task(struct task* task, pid_t blocked_by);
void syscall_rpc(dword callee_pid, dword data_size, char* data);

void syscall(struct int_stack_frame ** esp) 
{
    struct int_stack_frame * isf = *esp;
    
    //if(isf->eax != SYSCALL_KPRINTF)
        //kprintf("Syscall:%d\n", isf->eax);
        
    // FIXME: Adressen, die vom aufrufenden Programm kommen, 
    // vor der Verwendung prüfen
    switch (isf->eax) {
        case SYSCALL_KPRINTF:
            kprintf("%s", *((char**) isf->esp));
            break;

        case SYSCALL_MEM_ALLOCATE:
        {
            dword num;
            dword flags;

            paddr_t paddr;
            vaddr_t vaddr;

            num = PAGE_ALIGN_ROUND_UP(*((size_t*) isf->esp)) >> 12;
            flags = *((dword*) (isf->esp + 4));

            //kprintf("alloc %d Pages, flags %x\n", num, flags);
            
            
            // TODO: Flags berücksichtigen
            vaddr = find_contiguous_pages((page_directory_t) current_task->cr3, num, USER_MEM_START, USER_MEM_END);
            isf->eax = (dword) vaddr;
            
            //DMA-Page
            if((flags & 0x80) != 0)
            {
                paddr = phys_alloc_dma_page_range(num);
               
                //Bei DMA-Pages die phys. Adresse in ebx speichern
                isf->ebx = (dword) paddr;
                if (!map_page_range((page_directory_t) current_task->cr3, vaddr, paddr, PTE_P | PTE_U | PTE_W, num))
                {
                    panic("Fehler beim Zuweisen der reservierten DMA-Speicherseite %x", vaddr);
                }
            }
            else
            {
                while(num--)
                {
                    paddr = phys_alloc_page();
                    if (!map_page((page_directory_t) current_task->cr3, vaddr, paddr, PTE_P | PTE_U | PTE_W))
                    {
                        panic("Fehler beim Zuweisen der reservierten Speicherseite %x", vaddr);
                    }
                    vaddr += PAGE_SIZE;
                }
            }
            
            break;
        }

        case SYSCALL_MEM_FREE:
            break;

        case SYSCALL_PM_CREATE_PROCESS:
        {
            //TODO: Die UID irgendwie verwerten
            //uid_t uid = *((uid_t*) isf->esp + 4);
            
            struct task* new_task = create_task(*((void**) isf->esp));
            new_task->blocked_by_pid = current_task->pid;
            isf->eax = new_task->pid;
            break;
        }
        case SYSCALL_PM_INIT_PAGE:
        {
            struct task* new_task = get_task(*((pid_t*) isf->esp));
            
            if((new_task == NULL) || (new_task->blocked_by_pid != current_task->pid)) 
            {
                abort_task("SYSCALL_PM_INIT_PAGE: Der Aufrufer hat eine ungueltige PID angegeben: %d", *((pid_t*) isf->esp));
            }
            
            int num = PAGE_ALIGN_ROUND_UP(*((size_t*) (isf->esp + 12))) >> 12;
            vaddr_t dest = (vaddr_t) PAGE_ALIGN_ROUND_DOWN(*((size_t*) (isf->esp + 4)));
            vaddr_t src = (vaddr_t) PAGE_ALIGN_ROUND_DOWN(*((size_t*) (isf->esp + 8)));
            
            //kprintf("Map src:%x    dest:%x   size:%x", src, dest, num << 12);
            
            // Das PD-des neuen tasks mappen
            page_directory_t new_task_pd = map_phys_addr((paddr_t)new_task->cr3, PAGE_DIRECTORY_LENGTH);
            
            if (!map_page_range(new_task_pd, dest, resolve_vaddr((page_directory_t) current_task->cr3, src),  PTE_P | PTE_U | PTE_W, num)) {
                abort_task("SYSCALL_PM_INIT_PAGE: Fehler beim Zuweisen von %d reservierten Speicherseiten %x  =>  %x", num, src, dest);
            }

            /*while(num--) {
                if (!map_page(new_task_pd, dest, resolve_vaddr((page_directory_t) current_task->cr3, src), PTE_P | PTE_U | PTE_W)) {
                    panic("SYSCALL_PM_INIT_PAGE: Fehler beim Zuweisen der reservierten Speicherseite %x  =>  %x", src, dest);
                }
                
                src += 0x1000;
                dest += 0x1000;
            }*/
            
            free_phys_addr((vaddr_t)new_task_pd, PAGE_DIRECTORY_LENGTH);
            break;
        }
        case SYSCALL_PM_EXIT_PROCESS:
        {
            destroy_task(current_task);
            //timer_irq(0, (dword*) esp);
            schedule((dword*)esp);
            break;
        }
        case SYSCALL_PM_SLEEP:
            //timer_irq(0, (dword*) esp);
            schedule((dword*)esp);
            break;

        case SYSCALL_PM_GET_UID:
            isf->eax = 0;
            break;
        
        case SYSCALL_PM_GET_PID:
            isf->eax = current_task->pid;
            break;

        case SYSCALL_PM_P:
            block_task(current_task, current_task->pid);
            break;
        
        case SYSCALL_PM_V:
        {
            pid_t pid = *((pid_t*) isf->esp);
            struct task* task_ptr = get_task(pid);
            if(pid == 0)
            {
                unblock_task(current_task, current_task->pid);
            }
            else
            {
                if(task_ptr == NULL)
                    abort_task("SYSCALL_PM_V: Der Aufrufer hat eine ungueltige PID angegeben: %d", pid);
                
                if(task_ptr->blocked_by_pid == current_task->pid)
                {
                    if(unblock_task(task_ptr, current_task->pid) == FALSE)
                    {
                        panic("SYSCALL_PM_V: Der Task konnte nicht fortgesetzt werden");
                    }
                }
                else
                {
                    abort_task("SYSCALL_PM_V: Der Aufrufer hat einen Prozess(PID=%d) angegeben, der nicht von ihm blockiert ist", pid);
                }
            }
            break;
        }

        case SYSCALL_SET_RPC_HANDLER:
        {
            current_task->rpc_handler = *((vaddr_t*) isf->esp);
            break;
        }

        case SYSCALL_RPC:
        {
            syscall_rpc(
                *((dword*) isf->esp),
                *((dword*) (isf->esp + 4)),
                (char*) (isf->esp + 8)
            );
            break;
        }

        case SYSCALL_ADD_INTERRUPT_HANDLER:
        {
            set_intr_handling_task(*((dword*) isf->esp), current_task);
            break;
        }

        case SYSCALL_GET_TICK_COUNT:
        {
            extern unsigned long timer_ticks;
            isf->eax = (dword)timer_ticks;
            isf->ebx = (dword)(timer_ticks >> 32);
            break;
        }

        case SYSCALL_FORTY_TWO:
        {
            asm("int $0x20");
            char* forty_two = malloc(61);
            memcpy(forty_two, "\nThe Answer to Life, the Universe, and Everything is ... 42\n", 61);
            kprintf("%s", forty_two);
            break;
        }
        default:
            abort_task("Ungueltige Systemfunktion %d", isf->eax);
            break;
    }
}


void syscall_rpc(dword callee_pid, dword data_size, char* data) 
{
    struct task * callee = get_task(callee_pid);
    int i;
    
    if (!callee) {
        abort_task("RPC zu nicht vorhandenem Prozess %d", callee_pid);
        return;
    }

    if (data_size > PAGE_SIZE - 8) {
        abort_task("RPC-Datenblock zu gross: %d Bytes", data_size);
        return;
    }
    
    if (callee->rpc_handler == NULL) {
        // TODO Evtl. sollte eher der callee beendet werden, da von jedem Prozess erwartet werden kann, dass er einen RPC-Handler hat
        abort_task("RPC zu Prozess %d ohne RPC-Handler", callee_pid);
        return;
    }

    dword rounded_data_size = data_size;
    if((data_size % 4) != 0)
    {
        rounded_data_size += 4 - (data_size % 4);
    }

    dword caller_pid = current_task->pid;
    
    block_task (callee, caller_pid);
    
    // FIXME: Ganz grundsätzlich sollte nicht kernel_page_directory, sondern ein current_page_directory benutzt werden,
    // sonst funktioniert das nur zufällig
    dword callee_isf_phys = (dword) resolve_vaddr((page_directory_t) callee->cr3, (vaddr_t) callee->esp);
    struct int_stack_frame * callee_isf = find_contiguous_kernel_pages(1);
    map_page(kernel_page_directory, callee_isf, (paddr_t) PAGE_ALIGN_ROUND_DOWN(callee_isf_phys), PTE_P | PTE_W);
    callee_isf = (struct int_stack_frame *) ((dword) callee_isf + (callee_isf_phys % PAGE_SIZE));
    
    
/*    kprintf("syscall: RPC zu PID %d mit %d Bytes Daten;", callee_pid, data_size);
    kprintf("eip = %x, esp = %x, ", callee_isf->eip, callee_isf->esp);
    kprintf("esp0@%x=%x;size=0x%x;task@%x;cip=%x;cisf=%x;ccr3=%x\n", callee_isf, callee_isf->esp, data_size, callee, callee_isf_phys, callee->esp, callee->cr3);*/
    
    
	dword callee_esp_phys = (dword) resolve_vaddr((page_directory_t) callee->cr3, (vaddr_t) callee_isf->esp);
//    kprintf("callee_esp_phys = 0x%x\n", callee_esp_phys);

    dword pages_count = 1 + ((rounded_data_size + 20) / PAGE_SIZE) + (((rounded_data_size + 20) % PAGE_SIZE) > (callee_isf->esp % PAGE_SIZE) ? 1 : 0);
	if(resolve_vaddr((page_directory_t) callee->cr3, (vaddr_t) callee_isf->esp - (rounded_data_size + 20)) == NULL)
	{
        // TODO: Flexibler machen und nicht nur immer eine Page nehmen
		increase_user_stack_size(callee, 1);
		//kprintf("\033[1;37m\033[42m");
	}
	
    vaddr_t callee_esp = find_contiguous_kernel_pages(pages_count);
//    kprintf("%d freie Pages an %x\n", pages_count, callee_esp);
    vaddr_t callee_esp_kernel_page = callee_esp;
    for (i = 0; i < pages_count; i++) {
/*        kprintf("* %x => resolve(%x) ",
            (vaddr_t) ((dword) callee_esp + (PAGE_SIZE * i)), 
            (vaddr_t) callee_isf->esp - (PAGE_SIZE * (pages_count - i - 1))
        );*/
        map_page(
            kernel_page_directory, 
            (vaddr_t) ((dword) callee_esp + (PAGE_SIZE * i)), 
            (paddr_t) PAGE_ALIGN_ROUND_DOWN((dword) resolve_vaddr((page_directory_t) callee->cr3, (vaddr_t) callee_isf->esp - (PAGE_SIZE * (pages_count - i - 1)))),
            PTE_P | PTE_W
        );
    }

    callee_esp += (callee_esp_phys % PAGE_SIZE) + ((pages_count - 1) * PAGE_SIZE);
    callee_esp -= (rounded_data_size + 12);

    *((dword *) callee_esp) = data_size;
    *((dword *) (callee_esp + 4)) = caller_pid;
    memcpy(callee_esp + 8, data, data_size);
    *((dword *) (callee_esp + rounded_data_size + 8)) = callee_isf->eip;
    //kprintf("Saved RPC backlink 0x%x for task %d\n", callee_isf->eip, callee_pid);

    callee_isf->esp -= (rounded_data_size + 12);
    callee_isf->eip = (dword) callee->rpc_handler;
    //kprintf("=> c_esp=0x%x;esp1=0x%x\n", callee_esp, callee_isf->esp);
	
    for (i = 0; i < pages_count; i++) {
        unmap_page(kernel_page_directory, (vaddr_t) ((dword) callee_esp_kernel_page + (PAGE_SIZE * i)));
    }
    unmap_page(kernel_page_directory, (vaddr_t) PAGE_ALIGN_ROUND_DOWN((dword) callee_isf));
   
    unblock_task(callee, caller_pid);
}


void abort_task(char* format, ...)
{
    int * args = ((int*)&format) + 1;

    kprintf("\n"
      "\033[1;37m\033[41m" // weiss auf rot
      "Task %d beendet: ", current_task->pid);
    kaprintf(format, &args);
    kprintf("\n\033[0;37m\033[40m");

    destroy_task(current_task);
    while(1) {
        asm("sti; hlt");
    }
}
    
void block_task(struct task* task, pid_t blocked_by)
{
    unsigned long timeout = timer_ticks + 100;
    
    if(task->blocked_by_pid == blocked_by)
        return;

    while(task->blocked_by_pid)
    {
        asm("int $0x20");
    }
    
    task->blocked_by_pid = blocked_by;
}

bool unblock_task(struct task* task, pid_t blocked_by)
{
    if (task->blocked_by_pid == blocked_by) {
        task->blocked_by_pid = 0;
        return TRUE;
    } else {
        return FALSE;
    }
}
