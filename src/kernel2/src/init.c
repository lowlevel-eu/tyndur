/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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

#include <string.h>
#include <stdlib.h>

#include "multiboot.h"
#include "mm.h"
#include "im.h"
#include "tasks.h"

#include "console.h"
#include "kprintf.h"
#include "debug.h"
#include "apic.h"
#include "lock.h"

struct multiboot_info multiboot_info;
mmc_context_t init_context;

pm_thread_t init_thread;
pm_process_t init_process;

lock_t init_lock = 0;

void smp_init();
void gdt_init();

/** 
 * Kernel-Initialisierung erfolgt hier,
 * die Funktion init wird von header.asm aufgerufen 
 */
void init(int multiboot_magic, struct multiboot_info *boot_info)
{
    // Spielt nur bei SMP eine Rolle. Damit koennen die APs warten, bis der
    // Bootstrap-Prozessor fertig ist mit initialisieren.
    lock(&init_lock);

    init_console();

    memcpy(&multiboot_info, boot_info, sizeof(struct multiboot_info));

#if 0
    //Debugparameter verarbeiten
    if((multiboot_info.mi_flags & MULTIBOOT_INFO_HAS_CMDLINE) != 0)
    {
        debug_parse_cmdline(multiboot_info.mi_cmdline);
    }
#endif

    //debug_print(DEBUG_FLAG_INIT, "Initialisiere physikalische Speicherverwaltung");
    pmm_init(&multiboot_info);

    // SMP muss _direkt nach_ der physikalischen Speicherverwaltung
    // initialisiert werden, da sonst notwendige Strukturen im Speicher dazu
    // ueberschrieben werden koennten.
    smp_init();

    //debug_print(DEBUG_FLAG_INIT, "Initialisiere Paging");
    /*mmc_context_t kernel_context = mmc_create_kernel_context();
    vmm_init(kernel_context);*/
    init_thread.process = &init_process;
    cpu_get_current()->thread = &init_thread;

    
    // Interrupt-Manager vorbereiten und Interrupts aktiviernn
    im_init();
    im_enable();
    
    // Virtuelle Speicherverwaltung aktivieren
    init_context = mmc_create_kernel_context();
    cpu_get_current()->thread->process->context = init_context;
    vmm_init(&init_context);

    // GDT einrichten
    gdt_init();
    
    // Prozessverwaltung initialisieren
    pm_init();
    
    // Zum testen
    pm_process_t* kernel_process = pm_create((vaddr_t) NULL, "");
    //pm_thread_create(kernel_process, (vaddr_t) &kernel_thread0);
    //pm_thread_create(kernel_process, (vaddr_t) &kernel_thread1);

    // Den neu erstellen Kontext loeschen, und durch den Init-Kontext ersetzen
    mmc_destroy(&kernel_process->context);
    kernel_process->context = init_context;

    // Auf der aktuellen CPU wird der Kernel-Thread als laufender Thread
    // eingetragen.
    cpu_t* cpu = cpu_get_current();
    cpu->thread = list_get_element_at(kernel_process->threads, 0);
    cpu->thread->kernel_stack_bottom = vmm_kernel_automap(pmm_alloc(1), 
        0x1000);
    ((interrupt_stack_frame_t*) cpu->thread->kernel_stack)->esp = (uintptr_t) 
        cpu->thread->kernel_stack_bottom + 0x1000 - sizeof(
        interrupt_stack_frame_t);
    cpu->tss.esp0 = (uintptr_t) cpu->thread->kernel_stack_bottom; + 0x1000
        - sizeof(interrupt_stack_frame_t);
    
    load_init_module(&multiboot_info);
    load_init_module(&multiboot_info);
#if 0
    debug_print(DEBUG_FLAG_INIT, "Initialisiere Speicherverwaltung");
    init_memory_manager();
    
    debug_print(DEBUG_FLAG_INIT, "Initialisiere GDT");
    init_gdt();
    
    debug_print(DEBUG_FLAG_INIT, "Initialisiere Kernel Stack-Backtraces");
    init_stack_backtrace();
    
    debug_print(DEBUG_FLAG_INIT, "Initialisiere IDT");
    init_idt();

    debug_print(DEBUG_FLAG_INIT, "Initialisiere Scheduler");
    init_scheduler();
    
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
#endif
    unlock(&init_lock);
    while(1)
    {
        __asm__ __volatile__("sti");
    }
}

typedef __builtin_va_list va_list;
#define va_start(ap, f) __builtin_stdarg_start((ap),f)
#define va_arg __builtin_va_arg
#define va_end __builtin_va_end


__attribute__((noreturn)) void panic(char * message, ...)
{
    va_list args;
    va_start(args,message);

    // XXX disable_interrupts();

    kprintf("\n"
      "\033[1;37m\033[41m" // weiss auf rot
      "PANIC: ");
    kaprintf(message, (int*)args);
    kprintf("\n");

    #ifdef CONFIG_DEBUG_LAST_SYSCALL
    {
        dword i;

        kprintf("Letzter Syscall: %d ", debug_last_syscall_no);
        for (i = 0; i < DEBUG_LAST_SYSCALL_DATA_SIZE; i++) {
            kprintf("0x%08x ", debug_last_syscall_data[i]);
        }
        kprintf("\n");
    }
    #endif 

    stack_backtrace(0, 0);

    while(1) {
      __asm__ __volatile__("cli; hlt");
    }
}
