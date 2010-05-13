/*  
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Andreas Klebinger.
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

 /** 
  * Kernel-Initialisierung erfolgt hier,
  * die Funktion init wird von header.asm aufgerufen 
  */

#include <stdint.h>

#include "kernel.h"
#include "gdt.h"
#include "intr.h"
#include "kprintf.h"
#include "ports.h"
#include "multiboot.h"
#include "string.h"
#include "tss.h"
#include "kprintf.h"
#include "paging.h"
#include "modules.h"
#include "stdlib.h"
#include <lost/config.h>
#include "syscall.h"
#include "debug.h"
#include "io.h"
#include "timer.h"
#include "shm.h"
#include "vm86.h"

struct multiboot_info multiboot_info;

extern void init_phys_mem(vaddr_t mmap_addr, uint32_t mmap_length,
    uint32_t upper_mem);
extern void init_paging(void);
extern void init_console(void);
extern void init_scheduler(void);
extern void init_stack_backtrace(void);

void keyboard_handler(int irq)
{
    //kprintf("[KEYB]\n");
}

void init(int multiboot_magic, struct multiboot_info *boot_info)
{
    init_console();

    memcpy(&multiboot_info, boot_info, sizeof(struct multiboot_info));

    
    //Debugparameter verarbeiten
    if((multiboot_info.mi_flags & MULTIBOOT_INFO_HAS_CMDLINE) != 0)
    {
        debug_parse_cmdline(multiboot_info.mi_cmdline);
    }
    
    save_bios_data();
    
    debug_print(DEBUG_FLAG_INIT, "Initialisiere physikalische Speicherverwaltung");
    init_phys_mem(multiboot_info.mi_mmap_addr, multiboot_info.mi_mmap_length, multiboot_info.mi_mem_upper);

    debug_print(DEBUG_FLAG_INIT, "Initialisiere Paging");
    init_paging();

    debug_print(DEBUG_FLAG_INIT, "Initialisiere Speicherverwaltung");
    init_memory_manager();
    init_shared_memory();
    
    debug_print(DEBUG_FLAG_INIT, "Initialisiere GDT");
    init_gdt();
    
    debug_print(DEBUG_FLAG_INIT, "Initialisiere Kernel Stack-Backtraces");
    init_stack_backtrace();
    
    debug_print(DEBUG_FLAG_INIT, "Initialisiere IDT");
    init_idt();

    debug_print(DEBUG_FLAG_INIT, "Initialisiere Scheduler");
    init_scheduler();
    
    debug_print(DEBUG_FLAG_INIT, "Initialisiere Timer");
    timer_init();
    
    debug_print(DEBUG_FLAG_INIT, "Initialisiere IO-Ports");
    init_io_ports();
    
    
    //Erst das Init-Modul laden, danach Multitasking starten und erst dann die
    //Module an init weiter geben
    debug_print(DEBUG_FLAG_INIT, "Lade Init-Modul");
    load_init_module(&multiboot_info);

    debug_print(DEBUG_FLAG_INIT, "Uebergebe die restlichen Module an das Init-Modul");
    load_multiboot_modules(&multiboot_info);

    request_irq(1, &keyboard_handler);

    
    debug_print(DEBUG_FLAG_INIT, "Aktiviere Interrupts");
    enable_interrupts();

    // An dieser Stelle sofort einen Interrupt auszuführen, ist aus zwei 
    // Gründen notwendig:
    //
    // 1. Kooperatives Multitasking startet sonst nicht
    // 2. current_task sollte möglichst schnell initialisiert werden, da
    //    es nur an wenigen Stellen auf NULL überprüft wird
    asm volatile ("int $0x20");   
    
    while(1)
    {
        __asm__ __volatile__("hlt");
    }
}

extern void stack_backtrace(void);

__attribute__((noreturn)) void panic(char * message, ...)
{
    int * args = ((int*)&message) + 1;
    disable_interrupts();

    kprintf("\n"
      "\033[1;37m\033[41m" // weiss auf rot
      "PANIC: ");
    kaprintf(message, &args);
    kprintf("\n");

    #ifdef CONFIG_DEBUG_LAST_SYSCALL
    {
        uint32_t i;

        kprintf("Letzter Syscall (von PID %d): %d ", debug_last_syscall_pid,
            debug_last_syscall_no);
        for (i = 0; i < DEBUG_LAST_SYSCALL_DATA_SIZE; i++) {
            kprintf("0x%08x ", debug_last_syscall_data[i]);
        }
        kprintf("\n");
    }
    #endif 

    stack_backtrace();

    while(1) {
      __asm__ __volatile__("hlt");
    }
}

void syscall_p(void) {}
void syscall_v(void) {}
