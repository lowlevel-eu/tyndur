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

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <lost/config.h>
#include <lock.h>

#include "multiboot.h"
#include "mm.h"
#include "im.h"
#include "tasks.h"
#include "syscall.h"

#include "console.h"
#include "kprintf.h"
#include "debug.h"
#include "apic.h"
#include "lock.h"
#include "gdt.h"
#include "timer.h"
#include "lostio/core.h"

struct multiboot_info multiboot_info;

#define init_context (&init_thread.process->context)

extern const void kernelstack_bottom;
extern const void kernelstack;

pm_thread_t init_thread;
pm_process_t init_process = {
    .cmdline = "(Kernel)",
};

lock_t init_lock = 0;

void smp_init(void);
void gdt_init(void);
void vm86_init(void);
void panic(char* message, ...);

/**
 * Liest die Kernelkommandozeile ein
 *
 * Als erster Teilstring wird einfach irgendwas akzeptiert, weil davon
 * auszugehen ist, dass das der Kernelname ist. GRUB 2 übergibt den
 * Kernelnamen allerdings nicht, deswegen werden auch an erster Position
 * Parameter ausgewertet, wenn sie bekannt sind.
 */
static void parse_cmdline(char* cmdline)
{
    char* p;
    extern bool enable_smp;
    bool kernel_name = true;

    p = strtok(cmdline, " ");

    while (p) {
        if (!strncmp(p, "debug=", 6)) {
            debug_parse_cmdline(p);
        } else if (!strcmp(p, "smp")) {
            enable_smp = true;
        } else if (kernel_name) {
            kernel_name = false;
        } else {
            kprintf("Unbekannte Option: '%s'\n", p);
        }

        p = strtok(NULL, " ");
    }
}

/**
 * Kernel-Initialisierung erfolgt hier,
 * die Funktion init wird von header.asm aufgerufen (BSP = true) oder aus
 * smp_trampoline.asm
 *
 * @param multiboot_magic Multiboot Magic-Number
 * @param multiboot_info Pointer zur Multiboot-Info-Struct
 * @param bsp true, wenn die Funktion auf dem Bootstrap-Prozessor laeuft
 */
void init(int multiboot_magic, struct multiboot_info *boot_info, bool bsp)
{
    static lock_t init_lock = LOCK_LOCKED;;
    // Diese ersten Initialisierungen muessen nur einmal, und ganz am Anfang
    // gemacht werden.
    if (bsp == true) {
        init_console();

        // Multiboot-Infos sichern
        memcpy(&multiboot_info, boot_info, sizeof(struct multiboot_info));

        // Hiermit wird eine Laufende Multitasking-Umgebung simuliert. Dies ist
        // notwendig, damit im restlichen Code keine Aenderungen notwendig sind
        // fuer die Initialisierung.
        init_thread = (pm_thread_t) {
            .process                = &init_process,
            .kernel_stack_bottom    = (vaddr_t) &kernelstack_bottom,
            .kernel_stack_size      = (uint8_t*) &kernelstack
                                    - (uint8_t*) &kernelstack_bottom,
        };

        cpu_get(0)->thread = &init_thread;

        // Debugparameter verarbeiten, das ist aber nur moeglich, wenn eine
        // Kernelkommandozeile existiert.
        if ((multiboot_info.mi_flags & MULTIBOOT_INFO_HAS_CMDLINE) != 0) {
            parse_cmdline((char*)(uintptr_t)multiboot_info.mi_cmdline);
        }

#if CONFIG_ARCH == ARCH_I386
        // IVT fuer den VM86 sichern
        vm86_init();
#endif

        debug_print(DEBUG_FLAG_INIT, "Initialisiere physische "
            "Speicherverwaltung");
        pmm_init(&multiboot_info);

        // APIC initialisieren (Wird bei der SMP-Initialisierung genutzt
        apic_init();

        // SMP muss _direkt nach_ der physikalischen Speicherverwaltung
        // initialisiert werden, da sonst notwendige Strukturen im Speicher
        // dazu ueberschrieben werden koennten.
        debug_print(DEBUG_FLAG_INIT, "Initialisiere Mehrprozessor-"
            "Unterstuetzung");
        smp_init();

        // GDT und IDT vorbereiten
        debug_print(DEBUG_FLAG_INIT, "gdt/idt_init");
        gdt_init();
        im_init();

        // Eventuellen zusaetzlichen Prozessoren Mitteilen, dass sie mit der
        // Initialisierung fortfahren duerfen.
        unlock(&init_lock);
    } else {
        lock_wait(&init_lock);

        // APIC initialisieren. Dies ist beim BSP bereits erledigt.
        apic_init();
    }

    // Die IDT und die GDT wurden schon vorbereitet und muessen jetzt nur noch
    // aktiviert werden auf diesem Prozessor.
    debug_print(DEBUG_FLAG_INIT, "Initialisiere Deskriptortabellen fuer einen "
        "Prozessor");
    gdt_init_local();
    im_init_local();


    // Auf dem BSP muss die ganze virtuelle Speicherverwaltung initialisiert
    // werden. Bei den APs reicht es, wenn sie ihren Stack mappen.
    static volatile lock_t mm_prepare_lock = LOCK_LOCKED;
    cpu_get_current()->thread = &init_thread;
    if (bsp == true) {
        *init_context = mmc_create_kernel_context();
        mmc_activate(init_context);

        // Freigabe erteilen, damit die APs ihre Stacks mappen koennen.
        unlock(&mm_prepare_lock);
    } else {
        // Warten, bis wir das Signal vom BSP haben, zum fortfahren
        lock_wait(&mm_prepare_lock);

        // TODO: Was tun, wenn dieses identity-Mapping schief geht?
        // Dies ist naehmlich sher warscheinlich.
        uintptr_t sp;
        #if CONFIG_ARCH == ARCH_AMD64
            asm volatile("movq %%rsp, %0" : "=r"(sp));
        #else
            asm volatile("movl %%esp, %0" : "=r"(sp));
        #endif
        mmc_map(init_context, (vaddr_t) PAGE_ALIGN_ROUND_DOWN(sp),
            (paddr_t) PAGE_ALIGN_ROUND_DOWN(sp), MM_FLAGS_KERNEL_DATA, 1);
    }


    // Warten, bis alle CPUs hier angekommen sind.
    // TODO: Timeout?
    static volatile uint32_t cpus_arrived = 0;
    locked_increment(&cpus_arrived);
    while (cpus_arrived < cpu_count) asm("nop");

    // SSE aktivieren
    // Allerdings nur, wenn cpuid sagt, dass die Flags FXSR und SSE da sind,
    // sonst rennen wir hier in Exceptions.
    // TODO: Buildscripts fixen?
    uint32_t edx = 0, dummy = 0;
    asm volatile("cpuid": "=d" (edx), "=a" (dummy) : "a" (1) : "ebx", "ecx");
    if ((edx & (1 << 24)) && (edx & (1 << 25))) {
        #if CONFIG_ARCH == ARCH_AMD64
            asm volatile(
                "movq %cr0, %rax;"
                "andq $0xfffffffffffffffb, %rax;"
                "orq $2, %rax;"
                "movq %rax, %cr0;"
                "movq %cr4, %rax;"
                "orq $0x600, %rax;"
                "movq %rax, %cr4;"
            );
        #else
            asm volatile(
                "movl %cr0, %eax;"
                "andl $0xfffffffb, %eax;"
                "orl $2, %eax;"
                "movl %eax, %cr0;"
                "movl %cr4, %eax;"
                "orl $0x600, %eax;"
                "movl %eax, %cr4;"
            );
        #endif
    }

    static volatile lock_t vmm_lock = LOCK_LOCKED;
    // ACHTUNG: Hier besteht ein kleines Problem: Da in vmm_init der APIC
    // gemappt wird, kann nicht mehr darauf zugegriffen werden, bis Paging
    // aktiviert ist. Das geht sonst schief! Genau das geschieht aber, wenn
    // mapping-Funktionen aufgerufen werden. Darum muss alles gemappt sein,
    // bevor vmm_init aufgerufen wird.
    if (bsp == true) {
        debug_print(DEBUG_FLAG_INIT, "Initialisiere virtuelle "
            "Speicherverwaltung");
        vmm_init(init_context);
        unlock(&vmm_lock);
    }

    lock_wait(&vmm_lock);
    // Hier wird je nach Architektur Paging aktiviert
    vmm_init_local(init_context);


    // Hier geht die initialisierung langsam dem Ende zu. Jetzt fuehrt der BSP
    // noch ein paar notwendige Dinge durch, wartet dann auf die anderen CPUs
    // und gibt den endgueltigen Startschuss.
    static volatile uint32_t final_cpus_arrived = 0;
    static volatile lock_t final_lock = LOCK_LOCKED;
    locked_increment(&final_cpus_arrived);

    if (bsp == true) {
        debug_print(DEBUG_FLAG_INIT, "Initialisiere Kerneldienste");

        // Syscalls initialisieren
        syscall_init();

        // Prozessverwaltung initialisieren
        pm_init();

        // Shared Memory initialisieren
        shm_init();

        // Timer initialisieren
        timer_init();

        // LostIO initialisieren
        lio_init();

        // Init-Modul laden
        debug_print(DEBUG_FLAG_INIT, "Lade Module");
        load_init_module(&multiboot_info);

        // Alle weiteren Module an init uebergeben
        load_multiboot_modules(&multiboot_info);

        // Sobald alle CPUs angekommen sind, gehts los!
        // TODO: Timeout?
        debug_print(DEBUG_FLAG_INIT, "BSP: Warte auf andere CPUs");
        while (final_cpus_arrived < cpu_count) asm("nop");
        unlock(&final_lock);
    } else {
        // Warten, bis wir das Signal vom BSP haben, zum fortfahren
        lock_wait(&final_lock);
    }


    // Ersten Thread auf dieser CPU starten
    debug_print(DEBUG_FLAG_INIT, "Starte Ausfuehrung von init");
    current_thread = pm_scheduler_pop();
    interrupt_stack_frame_t* isf = im_prepare_current_thread();
    asm volatile("jmp im_run_thread" :: "a" (isf));

    // Hier kommen wir nie hin
}

__attribute__((noreturn)) void panic(char * message, ...)
{
    __asm__ __volatile__("cli");
    va_list args;
    va_start(args,message);

    // XXX disable_interrupts();

    kprintf("\n"
      "\033[1;37m\033[41m" // weiss auf rot
      "PANIC: ");
    kaprintf(message, args);
    kprintf("\n");

    // FIXME
    #undef CONFIG_DEBUG_LAST_SYSCALL
    #ifdef CONFIG_DEBUG_LAST_SYSCALL
    {
        uint32_t i;

        kprintf("Letzter Syscall: %d ", debug_last_syscall_no);
        for (i = 0; i < DEBUG_LAST_SYSCALL_DATA_SIZE; i++) {
            kprintf("0x%08x ", debug_last_syscall_data[i]);
        }
        kprintf("\n");
    }
    #endif

    stack_backtrace(0, 0);

    kprintf("\033[0;37m\033[40m");


    while(1) {
      cpu_halt();
    }
}
