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

extern void timer_irq(int irq, dword* esp);

void syscall(struct int_stack_frame ** esp) 
{
    struct int_stack_frame * isf = *esp;

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

           // kprintf("alloc %d Pages, flags %x\n", num, flags);
            
            
            // TODO: Flags berücksichtigen
            vaddr = find_contiguous_pages((page_directory_t) current_task->cr3, num, USER_MEM_START, USER_MEM_END);
            isf->eax = (dword) vaddr;

            while(num--) {
                paddr = phys_alloc_page();
                if (!map_page((page_directory_t) current_task->cr3, vaddr, paddr, PTE_P | PTE_U | PTE_W)) {
                    panic("Fehler beim Zuweisen der reservierten Speicherseite %x", vaddr);
                }
                vaddr += PAGE_SIZE;
            }
            
            break;
        }

        case SYSCALL_MEM_FREE:
            break;

		case SYSCALL_PM_SLEEP:
            timer_irq(0, (dword*) esp);
			break;

        case SYSCALL_SET_RPC_HANDLER:
        {
            current_task->rpc_handler = *((vaddr_t*) isf->esp);
            break;
        }

        case SYSCALL_RPC:
        {
            // TODO: Was bei Stack Overflow?
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
            
        default:
            // TODO: Task beenden statt hängen lassen
            kprintf("\n"
                "\033[1;37m\033[41m" // weiss auf rot
                "Task gestoppt: Ungueltige Systemfunktion %d\n"
                "\033[0;37m\033[40m"
                , isf->eax);
            while(1) { asm("hlt"); }
            break;
    }
}


void increase_user_stack_size(struct task * task_ptr, int pages)
{
	int i;
	for(i = 0; i < pages; i++)
	{
		task_ptr->user_stack_bottom -= PAGE_SIZE;
		if(resolve_vaddr((page_directory_t) task_ptr->cr3, (vaddr_t) task_ptr->user_stack_bottom) != NULL)
		{
			kprintf("\n"
                "\033[1;37m\033[41m" // weiss auf rot
                "Task gestoppt: Konnte den Stack nicht weiter vergroessern\n"
                "\033[0;37m\033[40m");
            while(1) { asm("hlt"); }
		}

        map_page((page_directory_t) task_ptr->cr3, task_ptr->user_stack_bottom, phys_alloc_page(), PTE_P | PTE_W | PTE_U);
	}
	
}

void syscall_rpc(dword callee_pid, dword data_size, char* data) {
    // TODO: Was bei Stack Overflow?

	
    struct task * callee = get_task(callee_pid);
    int i;

    if (data_size > PAGE_SIZE - 8) {
        kprintf("\n"
            "\033[1;37m\033[41m" // weiss auf rot
            "Task gestoppt: RPC-Datenblock zu gross: %d Bytes\n"
            "\033[0;37m\033[40m"
            , data_size);
        while(1) { asm("hlt"); }
    }
    
    if (callee->rpc_handler == NULL) {
        kprintf("\n"
            "\033[1;37m\033[41m" // weiss auf rot
            "Task gestoppt: RPC zu Prozess %d ohne RPC-Handler\n"
            "\033[0;37m\033[40m"
            , callee_pid);
       while(1) { asm("hlt"); }
    }

    dword caller_pid = get_pid(current_task);
    while(callee->blocked_by_pid);
    callee->blocked_by_pid = caller_pid;

    // FIXME: Ganz grundsätzlich sollte nicht kernel_page_directory, sondern ein current_page_directory benutzt werden,
    // sonst funktioniert das nur zufällig
    dword callee_isf_phys = (dword) resolve_vaddr((page_directory_t) callee->cr3, (vaddr_t) callee->esp);
    struct int_stack_frame * callee_isf = find_contiguous_kernel_pages(1);
    map_page(kernel_page_directory, callee_isf, (paddr_t) PAGE_ALIGN_ROUND_DOWN(callee_isf_phys), PTE_P | PTE_W);
    callee_isf = (struct int_stack_frame *) ((dword) callee_isf + (callee_isf_phys % PAGE_SIZE));
    
    /*
    kprintf("syscall: RPC zu PID %d mit %d Bytes Daten;", callee_pid, data_size);
    kprintf("eip = %x, esp = %x, ", callee_isf->eip, callee_isf->esp);
    kprintf("esp0@%x=%x;size=0x%x;task@%x;cip=%x;cisf=%x;ccr3=%x\n", callee_isf, callee_isf->esp, data_size, callee, callee_isf_phys, callee->esp, callee->cr3);
    */
    
	dword callee_esp_phys = (dword) resolve_vaddr((page_directory_t) callee->cr3, (vaddr_t) callee_isf->esp);
//    kprintf("callee_esp_phys = 0x%x\n", callee_esp_phys);

    dword pages_count = 1 + ((data_size + 20) / PAGE_SIZE) + (((data_size + 20) % PAGE_SIZE) > (callee_isf->esp % PAGE_SIZE) ? 1 : 0);
	if(resolve_vaddr((page_directory_t) callee->cr3, (vaddr_t) callee_isf->esp - (data_size + 20)) == NULL)
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
    callee_esp -= (data_size + 8);

    *((dword *) callee_esp) = data_size;
    memcpy(callee_esp + 4, data, data_size);
    *((dword *) (callee_esp + data_size + 4)) = callee_isf->eip;
    //kprintf("Saved RPC backlink 0x%x for task %d\n", callee_isf->eip, callee_pid);

    callee_isf->esp -= (data_size + 8);
    callee_isf->eip = (dword) callee->rpc_handler;
//    kprintf("=> c_esp=0x%x;esp1=0x%x\n", callee_esp, callee_isf->esp);
	
    for (i = 0; i < pages_count; i++) {
        unmap_page(kernel_page_directory, (vaddr_t) ((dword) callee_esp_kernel_page + (PAGE_SIZE * i)));
    }
    unmap_page(kernel_page_directory, (vaddr_t) PAGE_ALIGN_ROUND_DOWN((dword) callee_isf));
   
    if (callee->blocked_by_pid != caller_pid) {
        panic("Systemfunktion RPC: Konnte Prozess nicht reservieren");
    }
    callee->blocked_by_pid = 0;
}
