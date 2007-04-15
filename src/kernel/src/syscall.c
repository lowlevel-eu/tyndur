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
#include "phys.h"
#include "kernel.h"
#include "string.h"
#include "kmm.h"
#include "tasks.h"
#include "stdlib.h"
#include "intr.h"
#include "config.h"

extern void timer_irq(int irq, dword* esp);
extern qword timer_ticks;

#ifdef CONFIG_DEBUG_LAST_SYSCALL
dword debug_last_syscall_no = 0;
dword debug_last_syscall_data[DEBUG_LAST_SYSCALL_DATA_SIZE] = { 0 };
#endif 

void syscall(struct int_stack_frame ** esp) 
{
    struct int_stack_frame * isf = *esp;

    #ifdef CONFIG_DEBUG_LAST_SYSCALL
    {
        dword i;
        debug_last_syscall_no = isf->eax;
        for (i = 0; i < DEBUG_LAST_SYSCALL_DATA_SIZE; i++) {
            debug_last_syscall_data[i] = *(((dword*)isf->esp) + i);
        }
    }
    #endif 

    
    //if(isf->eax != SYSCALL_KPRINTF)
    //    kprintf("[PID %d] Syscall:%d\n", current_task->pid, isf->eax);
        
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

            //kprintf("\nMEM_ALLOCATE: PID %d: alloc %d Pages, flags %x\n", current_task->pid, num, flags);
            //stack_backtrace_ebp(isf->ebp, isf->eip);
            
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

                //kprintf("DMA-Adresse: phys 0x%08x, virt %08x\n", isf->ebx, isf->eax);
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
        {
            vaddr_t address = *((vaddr_t*) isf->esp);
            dword pages = PAGE_ALIGN_ROUND_UP(*((size_t*) (isf->esp + 4))) >> PAGE_SHIFT;
            
            //kprintf("\nMEM_FREE: PID %d: free  %x Pages @ 0x%x\n", current_task->pid, pages, address);
            
            if((dword)address % PAGE_SIZE != 0)
            {
                abort_task("SYSCALL_MEM_FREE: Der Task versucht eine nicht ausgerichtete Adresse freizugeben: 0x%08x", address);
            }

            for (; pages; pages--) {
                paddr_t paddress = resolve_vaddr((page_directory_t) current_task->cr3, address);

                if(unmap_page((page_directory_t) current_task->cr3, address) != TRUE)
                {
                    abort_task("SYSCALL_MEM_FREE: Die Speicherseite konnte nicht freigegeben werden: 0x%08x", address);
                }
                else
                {
                    phys_mark_page_as_free((void*) paddress);
                }

                address += PAGE_SIZE;
            }

            break;
        }
        
        case SYSCALL_MEM_INFO:
        {
            isf->eax = phys_count_pages() * PAGE_SIZE;
            isf->ebx = phys_count_free_pages() *PAGE_SIZE;

            break;
        }

        case SYSCALL_PM_CREATE_PROCESS:
        {
            //TODO: Die UID irgendwie verwerten
            //uid_t uid = *((uid_t*) isf->esp + 4);
            
            struct task* new_task = create_task(*((void**) isf->esp), *((const char**) ((dword)isf->esp + 8)));
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
            
            if (dest == NULL) {
                abort_task("SYSCALL_PM_INIT_PAGE: Versuchtes NULL-Mapping: src = 0x%08x", src);
            }
            
            
            //kprintf("Map psrc:%x    src:%x    dest:%x   size:%x\n", resolve_vaddr((page_directory_t) current_task->cr3, src), src, dest, num << 12);
            
            /*{
                dword i;
                for (i = 0; i < num; i++)
                    kprintf("[%08x %08x]\n", *((dword*) (src + 0x1000*i)), *((dword*) (src + 0x1000*i + 4)));
            }*/

            // Das PD-des neuen tasks mappen
            page_directory_t new_task_pd = new_task->cr3;
            
            // An dieser Stelle kann kein map_page_range benutzt werden, da
            // der Speicher nur virtuell, aber nicht unbedingt physisch
            // zusammenhängend sein muß.
            while(num--) {
                if (!map_page(new_task_pd, dest, resolve_vaddr((page_directory_t) current_task->cr3, src), PTE_P | PTE_U | PTE_W)) {
                    abort_task("SYSCALL_PM_INIT_PAGE: Fehler beim Zuweisen der reservierten Speicherseite %x  =>  %x", src, dest);
                }
                
                src += 0x1000;
                dest += 0x1000;
            }
            
            //free_phys_addr((vaddr_t)new_task_pd, PAGE_DIRECTORY_LENGTH);
            break;
        }
        case SYSCALL_PM_EXIT_PROCESS:
        {
            struct task* old_task = current_task;
            schedule((dword*)esp);
            destroy_task(old_task);
            break;
        }
        case SYSCALL_PM_SLEEP:
            schedule((dword*)esp);
            break;

        case SYSCALL_PM_GET_UID:
            isf->eax = 0;
            break;
        
        case SYSCALL_PM_GET_PID:
            isf->eax = current_task->pid;
            break;
        
        case SYSCALL_PM_GET_CMDLINE:
        {   
            vaddr_t vaddr = find_contiguous_pages((page_directory_t) current_task->cr3, 1, USER_MEM_START, USER_MEM_END);
            paddr_t paddr = phys_alloc_page();
            
            if (!map_page((page_directory_t) current_task->cr3, vaddr, paddr, PTE_P | PTE_U | PTE_W))
            {
                panic("Fehler beim Zuweisen der reservierten Speicherseite %x", vaddr);
            }

            isf->eax = (dword) vaddr;

            memcpy((void*) vaddr, (void*)(current_task->cmdline), strlen(current_task->cmdline) + 1);
            //kprintf("KERNEL return commmandline %s\n", current_task->cmdline);

            break;
        }
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
            if (syscall_rpc(
                *((dword*) isf->esp),
                *((dword*) (isf->esp + 4)),
                (char*) (isf->esp + 8)
            )) {
                isf->eax = 0;
            } else {
                isf->eax = -1;
                schedule((dword*)esp);
            }
            break;
        }

        case SYSCALL_ADD_INTERRUPT_HANDLER:
        {
            set_intr_handling_task(*((dword*) isf->esp), current_task);
            break;
        }

        case SYSCALL_GET_TICK_COUNT:
        {
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


bool syscall_rpc(dword callee_pid, dword data_size, char* data) 
{
    struct task * callee = get_task(callee_pid);
    int i;
    
    if (!callee) {
        abort_task("RPC zu nicht vorhandenem Prozess %d", callee_pid);
        return FALSE;
    }

    if (data_size > 8 * PAGE_SIZE - 8) {
        abort_task("RPC-Datenblock zu gross: %d Bytes", data_size);
        return FALSE;
    }
    
    if (callee->rpc_handler == NULL) {
        // TODO Evtl. sollte eher der callee beendet werden, da von jedem Prozess erwartet werden kann, dass er einen RPC-Handler hat
        abort_task("RPC zu Prozess %d ohne RPC-Handler", callee_pid);
        return FALSE;
    }

    dword rounded_data_size = data_size;
    if((data_size % 4) != 0)
    {
        rounded_data_size += 4 - (data_size % 4);
    }

    dword caller_pid = current_task->pid;
    
    if (!block_task (callee, caller_pid)) {
        return FALSE;
    }
    
    // FIXME: Ganz grundsätzlich sollte nicht kernel_page_directory, sondern ein current_page_directory benutzt werden,
    // sonst funktioniert das nur zufällig
    dword callee_isf_phys = (dword) resolve_vaddr((page_directory_t) callee->cr3, (vaddr_t) callee->esp);
    struct int_stack_frame * callee_isf = find_contiguous_kernel_pages(1);
    map_page(kernel_page_directory, callee_isf, (paddr_t) PAGE_ALIGN_ROUND_DOWN(callee_isf_phys), PTE_P | PTE_W);
    callee_isf = (struct int_stack_frame *) ((dword) callee_isf + (callee_isf_phys % PAGE_SIZE));
    
    
    //kprintf("syscall: RPC zu PID %d mit %d Bytes Daten;", callee_pid, data_size);
    /*kprintf("eip = %x, esp = %x, ", callee_isf->eip, callee_isf->esp);
    kprintf("esp0@%x=%x;size=0x%x;task@%x;cip=%x;cisf=%x;ccr3=%x\n", callee_isf, callee_isf->esp, data_size, callee, callee_isf_phys, callee->esp, callee->cr3);*/
    
    
	dword callee_esp_phys = (dword) resolve_vaddr((page_directory_t) callee->cr3, (vaddr_t) callee_isf->esp);
//    kprintf("callee_esp_phys = 0x%x\n", callee_esp_phys);

    dword pages_count = 1 + ((rounded_data_size + 20) / PAGE_SIZE) + (((rounded_data_size + 20) % PAGE_SIZE) > (callee_isf->esp % PAGE_SIZE) ? 1 : 0);
	if(resolve_vaddr((page_directory_t) callee->cr3, (vaddr_t) callee_isf->esp - (rounded_data_size + 20)) == NULL)
	{
        // TODO: Flexibler machen und nicht nur immer eine Page nehmen
		increase_user_stack_size(callee, 4);
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
    return TRUE;
}
