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

void syscall_rpc(dword callee_pid, dword data_size, char* data) {
    // TODO: Was bei Stack Overflow?

    struct task * callee = get_task(callee_pid);

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

    dword callee_esp_phys = (dword) resolve_vaddr((page_directory_t) callee->cr3, (vaddr_t) callee_isf->esp);
    vaddr_t callee_esp = find_contiguous_kernel_pages(1);
    map_page(kernel_page_directory, callee_esp, (paddr_t) PAGE_ALIGN_ROUND_DOWN(callee_esp_phys), PTE_P | PTE_W);
    callee_esp += (callee_esp_phys % PAGE_SIZE);
    callee_esp -= (data_size + 8);

    *((dword *) callee_esp) = data_size;
    memcpy(callee_esp + 4, data, data_size);
    *((dword *) (callee_esp + data_size + 4)) = callee_isf->eip;
    //kprintf("Saved RPC backlink 0x%x for task %d\n", callee_isf->eip, callee_pid);

    callee_isf->esp -= (data_size + 8);
    callee_isf->eip = (dword) callee->rpc_handler;

    unmap_page(kernel_page_directory, (vaddr_t) PAGE_ALIGN_ROUND_DOWN((dword) callee_esp));
    unmap_page(kernel_page_directory, (vaddr_t) PAGE_ALIGN_ROUND_DOWN((dword) callee_isf));
   
    if (callee->blocked_by_pid != caller_pid) {
        panic("Systemfunktion RPC: Konnte Prozess nicht reservieren");
    }
    callee->blocked_by_pid = 0;
}
