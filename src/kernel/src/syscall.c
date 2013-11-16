/*  
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
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

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

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
#include <lost/config.h>
#include "rpc.h"
#include "io.h"
#include "debug.h"
#include "syscall_structs.h"
#include "timer.h"
#include "shm.h"
#include "vm86.h"

extern void timer_irq(int irq, uint32_t* esp);
extern uint64_t timer_ticks;

extern void set_io_bitmap(void);

#ifdef CONFIG_DEBUG_LAST_SYSCALL
uint32_t debug_last_syscall_no = 0;
pid_t debug_last_syscall_pid = 0;
uint32_t debug_last_syscall_data[DEBUG_LAST_SYSCALL_DATA_SIZE] = { 0 };
#endif 

void syscall(struct int_stack_frame ** esp) 
{
    struct int_stack_frame * isf = *esp;

    #ifdef CONFIG_DEBUG_LAST_SYSCALL
    {
        uint32_t i;
        debug_last_syscall_no = isf->eax;
        debug_last_syscall_pid = current_task->pid;
        for (i = 0; i < DEBUG_LAST_SYSCALL_DATA_SIZE; i++) {
            debug_last_syscall_data[i] = *(((uint32_t*)isf->esp) + i);
        }
    }
    #endif 

    
    if (debug_test_flag(DEBUG_FLAG_SYSCALL)) {
        kprintf("[PID %d] Syscall:%d\n", current_task->pid, isf->eax);
        io_ports_check(current_task);
    }
        
    // FIXME: Adressen, die vom aufrufenden Programm kommen, 
    // vor der Verwendung prüfen
    switch (isf->eax) {
        case SYSCALL_PUTSN:
        {
            unsigned int n = *((int*) isf->esp);
            char * s = *((char**) (isf->esp + 4));

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
            uint32_t num;
            uint32_t flags;

            paddr_t paddr;
            vaddr_t vaddr;

            num = PAGE_ALIGN_ROUND_UP(*((size_t*) isf->esp)) >> 12;
            flags = *((uint32_t*) (isf->esp + 4));
            paddr_t* phys = *((paddr_t**) (isf->esp + 8));
            (void) flags;
            //kprintf("\nMEM_ALLOCATE: PID %d: alloc %d Pages, flags %x\n", current_task->pid, num, flags);
            //stack_backtrace_ebp(isf->ebp, isf->eip);
            
            // TODO: Flags berücksichtigen
            vaddr = find_contiguous_pages((page_directory_t) current_task->cr3, num, USER_MEM_START, USER_MEM_END);
            isf->eax = (uint32_t) vaddr;
            
            current_task->memory_used += num << 12;

            //DMA-Page
            if((flags & 0x80) != 0)
            {
                paddr = phys_alloc_dma_page_range_64k(num);
               
                //Bei DMA-Pages die phys. Adresse in ebx speichern
                *phys = paddr;
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
                    *phys = (paddr_t) NULL;
                    
                    if (!map_page((page_directory_t) current_task->cr3, vaddr, paddr, PTE_P | PTE_U | PTE_W))
                    {
                        panic("Fehler beim Zuweisen der reservierten Speicherseite %x", vaddr);
                    }
                    vaddr += PAGE_SIZE;
                }
            }
            
            break;
        }

        case SYSCALL_MEM_ALLOCATE_PHYSICAL:
        {
            uint32_t num;
            paddr_t position;
            uint32_t flags;

            vaddr_t vaddr;
            num = PAGE_ALIGN_ROUND_UP(*((size_t*) isf->esp)) >> 12;
            //TODO: Position entsprechend den Pages abrunden
            position = *((paddr_t*) (isf->esp + 4));
            flags = *((uint32_t*) (isf->esp + 8));
            (void) flags;
            // Speicher an der physikalischen Position einfach an eine freie
            // Position mappen
            vaddr = find_contiguous_pages((page_directory_t) current_task->cr3,
                num, USER_MEM_START, USER_MEM_END);
            isf->eax = (uint32_t) vaddr;

            current_task->memory_used += num << 12;
            
            //TODO: Speicher als benutzt markieren?
            //Nur nicht freien (=unnereichbaren) Speicher benutzen?
            if (!map_page_range((page_directory_t) current_task->cr3, vaddr,
                position, PTE_P | PTE_U | PTE_W, num)) {
                panic("Fehler beim Zuweisen des Speichers %x", vaddr);
            }
            break;
        }

        
        case SYSCALL_MEM_FREE:
        {
            vaddr_t address = *((vaddr_t*) isf->esp);
            uint32_t pages = PAGE_ALIGN_ROUND_UP(*((size_t*) (isf->esp + 4))) >> PAGE_SHIFT;
            
            //kprintf("\nMEM_FREE: PID %d: free  %x Pages @ 0x%x\n", current_task->pid, pages, address);
            
            current_task->memory_used -= pages << 12;

            if((uint32_t)address % PAGE_SIZE != 0)
            {
                if (debug_test_flag(DEBUG_FLAG_PEDANTIC)) {
                    abort_task("SYSCALL_MEM_FREE: Der Task versucht eine "
                        "nicht ausgerichtete Adresse freizugeben: 0x%08x", 
                        address);
                }
                isf->eax = false;
                break;
            }

            if (!is_userspace(address, pages * PAGE_SIZE)) 
            {
                if (debug_test_flag(DEBUG_FLAG_PEDANTIC)) {
                    abort_task("SYSCALL_MEM_FREE: Der Task versucht eine "
                        "Adresse freizugeben, die nicht fuer ihn reserviert "
                        "ist: 0x%08x (+0x%x)", address, pages * PAGE_SIZE);
                }
                isf->eax = false;
                break;
            }

            for (; pages; pages--) {
                paddr_t paddress = resolve_vaddr((page_directory_t) current_task->cr3, address);

                if(unmap_page((page_directory_t) current_task->cr3, address) != true)
                {
                    abort_task("SYSCALL_MEM_FREE: Die Speicherseite konnte nicht freigegeben werden: 0x%08x", address);
                }
                else
                {
                    phys_mark_page_as_free(paddress);
                }

                address += PAGE_SIZE;
            }

            isf->eax = true;
            break;
        }
        
        case SYSCALL_MEM_FREE_PHYSICAL:
        {
            vaddr_t address = *((vaddr_t*) isf->esp);
            uint32_t pages = PAGE_ALIGN_ROUND_UP(*((size_t*) (isf->esp + 4))) >> PAGE_SHIFT;
            current_task->memory_used -= pages << 12;
            if((uint32_t)address % PAGE_SIZE != 0)
            {
                abort_task("SYSCALL_MEM_FREE_PHYSICAL: Der Task versucht eine nicht ausgerichtete Adresse freizugeben: 0x%08x", address);
            }

            for (; pages; pages--) {
                //paddr_t paddress = resolve_vaddr((page_directory_t) current_task->cr3, address);

                if(unmap_page((page_directory_t) current_task->cr3, address) != true)
                {
                    abort_task("SYSCALL_MEM_FREE_PHYSICAL: Die Speicherseite konnte nicht freigegeben werden: 0x%08x", address);
                }
                //FIXME: Speicher freigeben?

                address += PAGE_SIZE;
            }

            break;
        }
        
        case SYSCALL_MEM_RESOLVE_VADDR:
        {
            vaddr_t address = *((vaddr_t*) isf->esp);
            if (is_userspace(address, 1)) {
                isf->eax = (uint32_t) resolve_vaddr(current_task->cr3, address);
            } else {
                isf->eax = 0;
            }

            break;
        }

        
        case SYSCALL_MEM_INFO:
        {
            isf->eax = phys_count_pages() * PAGE_SIZE;
            isf->edx = phys_count_free_pages() *PAGE_SIZE;

            break;
        }
        
        case SYSCALL_SHM_CREATE:
        {
            uint32_t size = *((uint32_t*) isf->esp);
            isf->eax = create_shared_memory(size);
            break;
        }

        case SYSCALL_SHM_ATTACH:
        {
            uint32_t id = *((uint32_t*) isf->esp);
            isf->eax = (uint32_t)attach_task_to_shm(current_task, id);
            break;
        }

        case SYSCALL_SHM_DETACH:
        {
            uint32_t id = *((uint32_t*) isf->esp);
            detach_task_from_shm(current_task, id);
            break;
        }

        case SYSCALL_PM_REQUEST_PORT:
        {
            uint32_t port = *((uint32_t*) isf->esp);
            uint32_t length = *((uint32_t*) (isf->esp + 4));

            isf->eax = io_ports_request(current_task, port, length);

            set_io_bitmap();
            break;
        }

        case SYSCALL_PM_RELEASE_PORT:
        {
            uint32_t port = *((uint32_t*) isf->esp);
            uint32_t length = *((uint32_t*) (isf->esp + 4));

            isf->eax = true;
            if (!io_ports_release(current_task, port, length)) 
            {
                if (debug_test_flag(DEBUG_FLAG_PEDANTIC)) {
                    abort_task("Freigab eines nicht reservierten Ports (port: 0x%x, Laenge 0x%x)", port, length);
                }

                isf->eax = false;
            }

            set_io_bitmap();
            break;
        }

        case SYSCALL_PM_CREATE_PROCESS:
        {
            //TODO: Die UID irgendwie verwerten
            //uid_t uid = *((uid_t*) isf->esp + 4);
            
            pid_t parent_pid = *((pid_t*)((uint32_t)isf->esp + 12));
            void* eip = *((void**) isf->esp);
            const char* cmdline = *((const char**) ((uint32_t)isf->esp + 8));

            struct task* new_task = create_task(eip, cmdline, parent_pid);

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
            
            current_task->memory_used -= num * PAGE_SIZE;
            //kprintf("Map psrc:%x    src:%x    dest:%x   size:%x\n", resolve_vaddr((page_directory_t) current_task->cr3, src), src, dest, num << 12);
            
            /*{
                uint32_t i;
                for (i = 0; i < num; i++)
                    kprintf("[%08x %08x]\n", *((uint32_t*) (src + 0x1000*i)), *((uint32_t*) (src + 0x1000*i + 4)));
            }*/

            // Das PD-des neuen tasks mappen
            page_directory_t new_task_pd = new_task->cr3;
            
            // An dieser Stelle kann kein map_page_range benutzt werden, da
            // der Speicher nur virtuell, aber nicht unbedingt physisch
            // zusammenhängend sein muß.
            while(num--) {
                if (!resolve_vaddr(new_task_pd, dest)) {
                    if (!map_page(new_task_pd, dest, resolve_vaddr((page_directory_t) current_task->cr3, src), PTE_P | PTE_U | PTE_W)) {
                        abort_task("SYSCALL_PM_INIT_PAGE: Fehler beim Zuweisen der reservierten Speicherseite %x  =>  %x", src, dest);
                    }
                } else {
                    // FIXME: Hier muss was passieren. Kopieren?
                }
                
                src += 0x1000;
                dest += 0x1000;
            }
            
            //free_phys_addr((vaddr_t)new_task_pd, PAGE_DIRECTORY_LENGTH);
            break;
        }
        case SYSCALL_PM_INIT_PAGE_COPY:
        {
            struct task* new_task = get_task(*((pid_t*) isf->esp));
            
            if((new_task == NULL) || (new_task->blocked_by_pid != current_task->pid)) 
            {
                abort_task("SYSCALL_PM_INIT_PAGE_COPY: Der Aufrufer hat eine ungueltige PID angegeben: %d", *((pid_t*) isf->esp));
            }

            int num = PAGE_ALIGN_ROUND_UP(*((size_t*) (isf->esp + 12))) >> 12;
            vaddr_t dest = (vaddr_t) PAGE_ALIGN_ROUND_DOWN(*((size_t*) (isf->esp + 4)));
            vaddr_t src = (vaddr_t) PAGE_ALIGN_ROUND_DOWN(*((size_t*) (isf->esp + 8)));
            
            if (dest == NULL) {
                abort_task("SYSCALL_PM_INIT_PAGE_COPY: Versuchtes Kopieren nach NULL: src = 0x%08x", src);
            }
            
            // Das PD-des neuen tasks mappen
            page_directory_t new_task_pd = new_task->cr3;
            
            // Speicher wird kopiert, nicht gemappt.
            while(num--) {
                if (!resolve_vaddr(new_task_pd, dest)) {
                    map_page(new_task_pd, dest, phys_alloc_page(), PTE_P | PTE_U | PTE_W);
                }
                
                paddr_t phys_addr = resolve_vaddr(new_task_pd, dest);
                char *destmem = find_contiguous_kernel_pages(1);
                map_page(kernel_page_directory, destmem, phys_addr, PTE_P | PTE_W);
                memcpy(destmem, src, 0x1000);
                unmap_page(kernel_page_directory, destmem);
                
                src += 0x1000;
                dest += 0x1000;
            }
            
            break;
        }
        case SYSCALL_PM_EXIT_PROCESS:
        {
            struct task* old_task = current_task;
            schedule((uint32_t*)esp);
            destroy_task(old_task);
            break;
        }
        
        case SYSCALL_PM_SLEEP:
            schedule((uint32_t*)esp);
            break;
        
        case SYSCALL_PM_WAIT_FOR_RPC:
            /*kprintf("Kernel: wait_for_rpc: PID %d an %08x\n", 
                current_task->pid, 
                isf->ebp >= 0x1000 ? *((uint32_t*) (isf->ebp  + 4)) : 0
            );*/
            current_task->status = TS_WAIT_FOR_RPC;
            schedule((uint32_t*)esp);
            break;
                    
        case SYSCALL_PM_V_AND_WAIT_FOR_RPC:
            if(unblock_task(current_task, current_task->pid) == false)
            {
                abort_task("SYSCALL_PM_V_AND_WAIT_FOR_RPC: "
                    "Der Task konnte nicht fortgesetzt werden");
            }
            /*kprintf("Kernel: v_and_wait_for_rpc: PID %d an %08x, blocked: %d\n",
                current_task->pid, 
                isf->ebp >= 0x1000 ? *((uint32_t*) (isf->ebp  + 4)) : 0,
                current_task->blocked_count
            );*/
            current_task->status = TS_WAIT_FOR_RPC;
            schedule((uint32_t*)esp);
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

            isf->eax = (uint32_t) vaddr;

            memcpy((void*) vaddr, (void*)(current_task->cmdline), strlen(current_task->cmdline) + 1);
            //kprintf("KERNEL return commmandline %s\n", current_task->cmdline);

            break;
        }

        case SYSCALL_PM_GET_PARENT_PID:
            // Eltern PID vom aktuellen Task
            if (*((pid_t*) isf->esp) == 0) {
                if (current_task->parent_task != NULL) {
                    isf->eax = current_task->parent_task->pid;
                } else {
                    isf->eax = 0;
                }
            } else {
                struct task* task = get_task(*((pid_t*) isf->esp));

                if (task == NULL) {
                    isf->eax = 0;
                } else {
                    if (task->parent_task == NULL) {
                        isf->eax = 0;
                    } else {
                        isf->eax = task->parent_task->pid;
                    }
                }
            }
            break;
        case SYSCALL_PM_ENUMERATE_TASKS:
        {
            // Erst werden die Tasks gezaehlt, und die Groesse der
            // Informationen mit den Kommandozeilen wird errechnet.
            size_t task_count = 0;
            size_t result_size = sizeof(task_info_t);
            struct task* task = first_task;
            uint32_t i;

            while (task != NULL) {
                task_count++;
                result_size += sizeof(task_info_task_t) + strlen(task->cmdline)
                    + 1;
                task = task->next_task;
            }
            
            // Anzahl der Seiten berechnen, die die Informationen benoetigen.
            size_t result_page_count = PAGE_ALIGN_ROUND_UP(result_size) /
                PAGE_SIZE;

            // Jetzt wird eine freie Stelle im Adressraum des Prozesses
            // gesucht, wo die Task-Infos hingemappt werden koennen
            task_info_t* task_info = find_contiguous_pages((page_directory_t)
                current_task->cr3, result_page_count, USER_MEM_START,
                USER_MEM_END);

            // Ein paar Seite an die Stelle mappen
            for (i = 0; i < result_page_count; i++) {
                if (!map_page((page_directory_t) current_task->cr3, (void*)
                    ((uint32_t)task_info + i * PAGE_SIZE), phys_alloc_page(), 
                    PTE_P | PTE_U | PTE_W))
                {
                    panic("Fehler beim Zuweisen der reservierten "
                        "Speicherseite %x", (uint32_t)task_info + i * PAGE_SIZE);
                }
            }
            
            // Der groessen-Eintrag ist nur da, damit der Tasks die Pages
            // freigeben koennte.
            task_info->info_size = result_size;

            task_info->task_count = task_count;

            // Dieser Pointer zeigt direkt hinter das Array mit den
            // Task-Informationen. Dort werden die Kommandozeilen
            // hintereinander gespeichert, und aus den Task-Strukturen wird auf
            // sie verwiesen.
            char* cmdlines = (char*) task_info->tasks;
            cmdlines += task_count * sizeof(task_info_task_t);
            
            task = first_task;
            // Jetzt werden die Infos eingefuellt.
            for (i = 0; i < task_count; i++) {
                task_info->tasks[i].pid = task->pid;
                task_info->tasks[i].status = task->status;
                task_info->tasks[i].eip = 
                    ((struct int_stack_frame*) task->esp)->eip;

                // Wenn der Task keinen Eltern-Task hat muessen wir aufpassen,
                // damit wir keinen Pagefault produzieren.
                if (task->parent_task == NULL) {
                    task_info->tasks[i].parent_pid = 0;
                } else {
                    task_info->tasks[i].parent_pid = task->parent_task->pid;
                }
                
                // Die Kommandozeile inklusive Nullbyte kopieren
                size_t cmdline_size = strlen(task->cmdline) + 1;
                memcpy(cmdlines, task->cmdline, cmdline_size);

                // Den Pointer fuer die Kommandozeile setzen
                task_info->tasks[i].cmdline = cmdlines;

                // Den Zielpointer fuer die naechste Kommandozeile direkt
                // hinter die aktuelle setzen.
                cmdlines += cmdline_size;
                
                task_info->tasks[i].memory_used = task->memory_used;
                task = task->next_task;
            }
            
            // In eax wird ein Pointer auf die Daten zurueck gegeben
            isf->eax = (uint32_t) task_info;

            break;
        }

        case SYSCALL_PM_P:
            /*kprintf("Kernel: p: PID %d an %08x => %d\n", 
                current_task->pid, 
                isf->ebp >= 0x1000 ? *((uint32_t*) (isf->ebp  + 4)) : 0,
                current_task->blocked_count
            );*/
            if (!block_task(current_task, current_task->pid)) {
                panic("Konnte Task nicht blockieren");
            }
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
                    if(unblock_task(task_ptr, current_task->pid) == false)
                    {
                        panic("SYSCALL_PM_V: Der Task konnte nicht fortgesetzt werden");
                    }
                }
                else
                {
                    abort_task("SYSCALL_PM_V: Der Aufrufer hat einen Prozess(PID=%d) angegeben, der nicht von ihm blockiert ist", pid);
                }
            }
            /*kprintf("Kernel: v: PID %d an %08x => %d\n", 
                current_task->pid, 
                isf->ebp >= 0x1000 ? *((uint32_t*) (isf->ebp  + 4)) : 0,
                current_task->blocked_count
            );*/
            break;
        }

        case SYSCALL_SET_RPC_HANDLER:
        {
            current_task->rpc_handler = *((vaddr_t*) isf->esp);
            break;
        }

        case SYSCALL_FASTRPC:
        {
            uint32_t metadata_size = *((uint32_t*) (isf->esp + 4));
            void* metadata = *((void**) (isf->esp + 8));
            uint32_t data_size = *((uint32_t*) (isf->esp + 12));
            void* data = *((void**) (isf->esp + 16));

            uint32_t callee_pid = *((uint32_t*) isf->esp);
            struct task* callee = get_task(callee_pid);

            if (!callee) {
                isf->eax = -ESRCH;
                break;
            }

            if (!is_userspace(metadata, metadata_size)) {
                abort_task("SYSCALL_FASTRPC: Ungueltige Metadaten (%08x)",
                    (uint32_t) metadata);
                break;
            }
            
            if (!is_userspace(data, data_size)) {
                abort_task("SYSCALL_FASTRPC: Ungueltige Daten (%08x)",
                    (uint32_t) data);
                break;
            }

            isf->eax = 0;
            if (fastrpc(
                callee,
                metadata_size, metadata,
                data_size, data    
            )) {
                schedule_to_task(callee, (uint32_t*) esp);
            } else {
                isf->eax = -EAGAIN;
                schedule((uint32_t*)esp);
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
            set_intr_handling_task(*((uint32_t*) isf->esp), current_task);
            break;
        }
        
        case SYSCALL_GET_TICK_COUNT:
        {
            isf->eax = (uint32_t)timer_ticks;
            isf->edx = (uint32_t)(timer_ticks >> 32);
            break;
        }

        case SYSCALL_ADD_TIMER:
        {
            timer_register(
                current_task,
                *((uint32_t*) (isf->esp)),
                *((uint32_t*) (isf->esp + 4))
            );
            break;
        }
        
        case SYSCALL_VM86:
        {
            vm86_regs_t *regs = *((vm86_regs_t **) (isf->esp));
            uint32_t *mem = *((uint32_t **) (isf->esp + 4));
            if (!regs) {
                isf->eax = 0;
                break;
            }
            if (create_vm86_task(0x10, regs, mem, current_task)) {
                isf->eax = 1;
            } else {
                isf->eax = 0;
            }
            schedule((uint32_t*)esp);
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

        case SYSCALL_DEBUG_STACKTRACE:
        {
            pid_t pid = *((uint32_t*) (isf->esp));
            struct task* task = get_task(pid);

            // FIXME: Was bei einem Fehler
            if (task != NULL) {
                // TODO: Irgend ein Gefuehl sagt mir, dass das nicht so einfach
                //       sein kann *g*
                // In den Kontext dieses Tasks wechseln
                __asm("mov %0, %%cr3" : : "r"(resolve_vaddr(kernel_page_directory, task->cr3)));            
                struct int_stack_frame* isf = (struct int_stack_frame*) task->esp;
                stack_backtrace_ebp(isf->ebp, isf->eip);

                // Zurueck in den Kontext des aktuellen Tasks wechseln
                __asm("mov %0, %%cr3" : : "r"(resolve_vaddr(kernel_page_directory, current_task->cr3)));
            }

            break;
        }

        default:
            abort_task("Ungueltige Systemfunktion %d", isf->eax);
            break;
    }
}
