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

#include "console.h"
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
#include "rpc.h"
#include "io.h"

extern void timer_irq(int irq, dword* esp);
extern qword timer_ticks;

extern void set_io_bitmap();

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
        case SYSCALL_PUTSN:
        {
            unsigned int n = ((int*) isf->esp)[1];
            char * s = *((char**) isf->esp);

            if(n == -1)
            {
                con_puts(s);
            }
            else
            {
                con_putsn(n, s);
            }
            break;
        }

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

        case SYSCALL_PM_REQUEST_PORT:
        {
            dword port = *((dword*) isf->esp);
            dword length = *((dword*) (isf->esp + 4));
            io_ports_request(current_task, port, length);
            set_io_bitmap();
            break;
        }

        case SYSCALL_PM_RELEASE_PORT:
        {
            dword port = *((dword*) isf->esp);
            dword length = *((dword*) (isf->esp + 4));
            io_ports_release(current_task, port, length);
            set_io_bitmap();
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

        case SYSCALL_FASTRPC:
        {
            struct task* callee = get_task(*((dword*) isf->esp));

            isf->eax = 0;
            if (fastrpc(
                callee,
                *((dword*) (isf->esp + 4)),
                *((void**) (isf->esp + 8)),
                *((dword*) (isf->esp + 12)),
                *((void**) (isf->esp + 16))
            )) {
                schedule_to_task(callee, (dword*) esp);
            } else {
                isf->eax = -1;
                schedule((dword*)esp);
            }
            break;
        }

        case SYSCALL_FASTRPC_RET:
        {
            return_from_rpc(esp);
            break;
        }

        case SYSCALL_ADD_INTERRUPT_HANDLER:
        {
            set_intr_handling_task(*((dword*) isf->esp), current_task);
            break;
        }
        
        case SYSCALL_RPC_PUSH_EFLAGS:
        {
            current_task->eflags_stack = list_push(current_task->eflags_stack, (void*) isf->eflags);
            break;
        }
        
        case SYSCALL_RPC_POP_EFLAGS:
        {
           dword new_eflags = (dword) list_pop(current_task->eflags_stack);
           
           if(new_eflags != 0)
           {
                isf->eflags = new_eflags;
           }
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
