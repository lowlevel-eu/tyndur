/*
 * Copyright (c) 2010 The tyndur Project. All rights reserved.
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
#include <stdlib.h>
#include <string.h>
#include <syscall_structs.h>
#include <errno.h>
#include <ports.h>

#include "kernel.h"
#include "kprintf.h"
#include "tasks.h"
#include "cpu.h"
#include "vm86.h"
#include "mm.h"

/*
 * Grundsaetzliche Funktionsweise von VM86 in tyndur:
 *
 * Die VM86-Funktionalitaet wird ausschliesslich ueber den Syscall
 * SYSCALL_VM86_BIOS_INT bereitgestellt. Beim Aufruf dieses Syscalls wird der
 * aufrufende Thread blockiert und zu einem neu erstellten VM86-Thread
 * desselben Prozesses gewechselt. Dieser Thread laeuft bis der BIOS-Interrupt
 * ueber iret zurueckkehrt, dann wird der VM86-Thread beendet und der
 * urspruengliche Thread fortgesetzt.
 *
 * Einsprung in den VM86:
 *
 *      Um in den VM86-Modus zu wechseln wird derselbe Mechanismus wie beim
 *      normalen Multitasking verwendet (d.h. Softwaremultitasking). Beim
 *      Sprung in den Task muss das VM-Flag in eflags gesetzt sein (0x20000),
 *      damit der Prozessor in den VM86 wechselt. Zusaetzlich zu den normal
 *      gesicherten Registern (esp, eflags, cs, eip) werden bei VM86-Tasks auch
 *      die (Real-Mode-)Segmentregister auf den Stack gesichert und beim iret
 *      wiederhergestellt.
 *
 * Behandlung privilegierter Instruktionen:
 *
 *      Wenn der VM86-Task auf privilegierte Instruktionen stoesst, wird ein
 *      #GP ausgeloest. vm86_exception() faengt diese Exception vor den
 *      normalen Exceptionhandlern ab und emuliert den aktuellen privilegierten
 *      Befehl wenn moeglich.
 *
 *      Bei der Emulation von iret ist zu beachten, dass der VM86-Task beendet
 *      werden muss, wenn sich der Stack wieder auf dem Anfangswert befindet;
 *      der aufgerufene Interrupthandler ist dann fertig.
 */

/** Enthaelt eine Kopie der urspruenglichen ersten 4k im physischen Speicher */
static struct {
    uint16_t    ivt[256][2];
    uint8_t     data[3072];
} bios_data __attribute__ ((aligned (4096)));

// FIXME Das darf eigentich nicht global sein
static struct {
    bool            in_use;
    void*           first_mb;
    uint32_t*       memory;
    pm_thread_t*    caller;
    vm86_regs_t*    regs;
    mmc_context_t   mmc;
} vm86_status = {
    .in_use     =   false,
};

extern uint16_t next_entry_index;
extern lock_t gdt_lock;
extern uint64_t idt[256];
extern uint64_t gdt[255];

extern void gdt_set_descriptor_byte_granularity(int segment, uint32_t size,
    uint32_t base, uint8_t access, int dpl);
extern void* gdt_get_descriptor_base(int segment);
extern void gdt_set_busy_flag(int segment, bool state);

extern interrupt_stack_frame_t* im_handler(interrupt_stack_frame_t* isf);
extern void load_isf_and_return(uint32_t new_stack) __attribute__((noreturn));

/**
 * Speichert BIOS-Daten, um sie den VM86-Tasks später bereitstellen zu können
 */
void vm86_init(void)
{
    memcpy(&bios_data, 0, 4096);
}

// Einsprungspunkt für den vm86-Thread nach jedem Taskwechsel (setzt das
// NT-Flag und biegt den Backlinkpointer vom aktuellen TSS um)
extern void vm86_task_entry(void);

/**
 * Gibt einen Pointer zum Backlink des Standard-TSS zurück
 */
uint32_t vm86_get_backlink_pointer(void)
{
    return (uintptr_t) &cpu_get_current()->tss.backlink;
}

static void vm86_gpf_entry(uint32_t above_error_code)
{
    // Der tatsächliche Fehlercode ist dort, wo normalerweise EIP für den
    // Rücksprung liegt, also genau ein uint32_t unter dem ersten Parameter
    uint32_t error_code = (&above_error_code)[-1];
    uint16_t tr;

    asm volatile ("str %0" : "=r"(tr));

    cpu_tss_t* gpf_tss = gdt_get_descriptor_base(tr >> 3);
    cpu_tss_t* vm86_tss = gdt_get_descriptor_base(gpf_tss->backlink >> 3);

    // Originalen Busyflagzustand wiederherstellen
    gdt_set_busy_flag(tr >> 3, false);
    gdt_set_busy_flag(gpf_tss->backlink >> 3, true);

    // Das Standard-TSS verwenden
    asm volatile ("ltr %0" :: "r"(cpu_get_current()->tss_selector));

    asm volatile ("pushfl;"
        "pop %%eax;"
        "and $0xFFFFBFFF,%%eax;" // NT-Flag löschen
        "push %%eax;"
        "popfl" ::: "eax");

    // Der „künstliche“ Stackframe hier enthält zum Teil Werte, die nichts mit
    // vm86 zu tun haben. Sollte der Thread unterbrochen werden, so wird die
    // Ausführung beim nächsten Mal bei vm86_task_entry fortgeführt.
    interrupt_stack_frame_t isf = {
        .eax = vm86_tss->eax,
        .ebx = gpf_tss->backlink,
        .ecx = vm86_tss->ecx,
        .edx = vm86_tss->edx,
        .esi = vm86_tss->esi,
        .edi = vm86_tss->edi,
        .ebp = (uintptr_t) vm86_tss,
        .esp = vm86_tss->esp,
        .ds = 0x10,
        .es = 0x10,
        .fs = 0x10,
        .gs = 0x10,
        .ss = 0x10,
        // Originale Interruptnummer wird aus dem Fehlercode bestimmt
        .interrupt_number = (error_code & (1 << 1)) ? (error_code >> 3) : 13,
        // Wenn dies ein reiner GPF ist, dann können wir den Fehlercode
        // übernehmen, sonst ist er unbekannt.
        .error_code = (error_code & (1 << 1)) ? 0 : error_code,
        .eip = (uintptr_t) &vm86_task_entry,
        .cs = 0x08,
        // Das gelöschte Bit 1 zeigt, dass es sich um einen vm86-Task handelt
        .eflags = 0x200
    };

    if ((isf.interrupt_number < 0x20) && (isf.interrupt_number != 13)) {
        // Das wird eine Exception, die eher nicht abgefangen werden dürfte.
        // Also setzen wir die Registerwerte korrekt, damit man auch was vom
        // Rot hat.
        isf.ebx = vm86_tss->ebx;
        isf.ebp = vm86_tss->ebp;
        isf.eip = vm86_tss->eip;
        isf.cs = vm86_tss->cs;
        isf.ds = vm86_tss->ds;
        isf.es = vm86_tss->es;
        isf.fs = vm86_tss->fs;
        isf.gs = vm86_tss->gs;
        isf.ss = vm86_tss->ss;
        isf.eflags = vm86_tss->eflags;
    }

    asm volatile ("mov %%cr0,%%eax;"
        "and $0xFFFFFFF7,%%eax;" // TS-Flag löschen (FPU sollte benutzbar bleiben)
        "mov %%eax,%%cr0" ::: "eax");

    load_isf_and_return((uintptr_t) im_handler(&isf));
}

/**
 * Erstellt einen VM86-Thread im aktuellen Prozess. Der aktuell ausgefuehrte
 * Thread wird pausiert und ein Taskwechsel zum neuen VM86-Task wird
 * durchgefuehrt.
 *
 * @oaram int Aufzurufender BIOS-Interrupt
 * @param regs Anfaengliche Registerbelegung
 * @param stack Virtuelle Adresse der fuer den Stack des VM86-Tasks benutzten
 * Seite
 */
static int create_vm86_task(int intr, vm86_regs_t* regs, uintptr_t stack)
{
    uint16_t* ivt_entry = bios_data.ivt[intr];

    // Erst einmal einen ganz normalen Thread erzeugen
    pm_thread_t* task = pm_thread_create(current_process,
        &vm86_task_entry);

    cpu_tss_t* vm86_tss = mmc_valloc_limits(&mmc_current_context(),
        NUM_PAGES(2 * sizeof(cpu_tss_t)), 0, 0,
        0x100000, KERNEL_MEM_END, MM_FLAGS_KERNEL_DATA);
    cpu_tss_t* gpf_tss = vm86_tss + 1;

    lock(&gdt_lock);

    int vm86_tss_index = next_entry_index++;
    gdt_set_descriptor_byte_granularity(vm86_tss_index, 2 * sizeof(*vm86_tss)
        - 1, (uintptr_t) vm86_tss, 0x8B, 0);

    int gpf_tss_index = next_entry_index++;
    gdt_set_descriptor_byte_granularity(gpf_tss_index, 2 * sizeof(*gpf_tss) - 1,
        (uintptr_t) gpf_tss, 0x89, 0);

    unlock(&gdt_lock);

    interrupt_stack_frame_t* isf = task->user_isf;
    isf->cs = 0x08;
    isf->ds = isf->es = 0x10;
    isf->ebx = vm86_tss_index << 3;

    vm86_tss->esp0 = (uintptr_t) isf;
    vm86_tss->ss0 = 0x10;

    // TODO: Das Folgende ist sehr i386- und 4-kB-Page-lastig
    mmc_context_t vm86_mmc = mmc_create_empty();
    mmc_context_t crnt_mmc = mmc_current_context();

    vm86_status.mmc = vm86_mmc;

    // Sorgt dafür, dass die erste Pagetable vorhanden ist.
    mmc_map(&vm86_mmc, (vaddr_t) PAGE_SIZE, (paddr_t) 0, 0, 1);

    page_table_t vm86_pt0 = get_pagetable(&vm86_mmc, 0);

    uintptr_t first_mb = (uintptr_t) vm86_status.first_mb;
    int pde = first_mb >> 22;

    page_table_t crnt_pt = get_pagetable(&crnt_mmc, pde);

    int page, pte = (first_mb >> 12) & 0x3FF;
    for (page = 0; page < 256; page++) {
        vm86_pt0[page] = crnt_pt[pte];

        if (++pte >= 1024) {
            free_pagetable(&crnt_mmc, crnt_pt);
            crnt_pt = get_pagetable(&crnt_mmc, ++pde);
            pte -= 1024;
        }
    }

    free_pagetable(&crnt_mmc, crnt_pt);
    free_pagetable(&vm86_mmc, vm86_pt0);

    // Wird auf i386 nicht größer (wenn wir hier einigermaßen unabhängig
    // sein wöllten, wirds spätestens beim uint64_t* schwierig
    paddr_t idt_phys = pmm_alloc(1);
    mmc_map(&vm86_mmc, idt, idt_phys, PTE_P | PTE_W, 1);

    uint64_t* vm86_idt = vmm_kernel_automap(idt_phys, PAGE_SIZE);
    int i;
    for (i = 0; i < 256; i++) {
        switch (i) {
            case 13:
                vm86_idt[i] = (uint64_t)
                    (gpf_tss_index << 19) | (5LL << 40) | (1LL << 47);
                break;
            default:
                vm86_idt[i] = 0;
        }
    }

    vmm_kernel_unmap(vm86_idt, PAGE_SIZE);

    paddr_t gdt_phys = mmc_resolve(&crnt_mmc, gdt);
    mmc_map(&vm86_mmc, gdt, gdt_phys, PTE_P | PTE_W,
        (sizeof(gdt) + PAGE_SIZE - 1) / PAGE_SIZE);

    vm86_tss->cr3 = (uintptr_t) vm86_mmc.page_directory;
    vm86_tss->eip = ivt_entry[0];
    vm86_tss->eflags = 0x20202;
    vm86_tss->eax = regs->ax;
    vm86_tss->ebx = regs->bx;
    vm86_tss->ecx = regs->cx;
    vm86_tss->edx = regs->dx;
    vm86_tss->esi = regs->si;
    vm86_tss->edi = regs->di;
    vm86_tss->esp = 0xFFFE;
    vm86_tss->ebp = 0;
    vm86_tss->cs = ivt_entry[1];
    vm86_tss->ds = regs->ds;
    vm86_tss->es = regs->es;
    vm86_tss->ss = (stack - 65536) >> 4;

    gpf_tss->esp0 = (uintptr_t) task->user_isf;
    gpf_tss->ss0 = 0x10;
    gpf_tss->cr3 = (uintptr_t) crnt_mmc.page_directory;
    gpf_tss->eip = (uintptr_t) &vm86_gpf_entry;
    gpf_tss->eflags = 0x2;
    gpf_tss->esp = gpf_tss->esp0;

    gpf_tss->cs = 0x08;
    gpf_tss->ds = 0x10;
    gpf_tss->es = 0x10;
    gpf_tss->fs = 0x10;
    gpf_tss->gs = 0x10;
    gpf_tss->ss = gpf_tss->ss0;

    uint8_t* tss_base = (uint8_t*) vm86_tss;
    int base_offset = (uintptr_t) tss_base % PAGE_SIZE;

    paddr_t tss_phys = mmc_resolve(&crnt_mmc, tss_base - base_offset);
    mmc_map(&vm86_mmc, tss_base - base_offset, tss_phys, PTE_P | PTE_W, 1);

    i = PAGE_SIZE - base_offset;
    while (i < 2 * sizeof(cpu_tss_t)) {
        paddr_t tss_phys = mmc_resolve(&crnt_mmc, tss_base + i);
        mmc_map(&vm86_mmc, tss_base + i, tss_phys, PTE_P | PTE_W, 1);

        i += PAGE_SIZE;
    }

    // Sofort in den VM86-Task wechseln. Der aufrufende Thread wird
    // waehrenddessen nicht in den Scheduler zurueckgegeben und gestoppt.
    current_thread->status = PM_STATUS_BLOCKED;
    current_thread = task;
    task->status = PM_STATUS_RUNNING;

    return 0;
}

/**
 * Implementiert den Syscall SYSCALL_VM86_BIOS_INT.
 *
 * @param intr BIOS-Interrupt, der aufgerufen werden soll
 * @param regs Pointer auf die Struktur, die die anfaenglichen Registerwerte
 * enthaelt und in die beim Ende des VM86-Tasks die dann aktuellen
 * Registerwerte kopiert werden sollen.
 * @param memory Array von Speicherbereichen, die in den VM86-Task gemappt
 * werden sollen; NULL fuer keine.
 *
 * @return 0 bei Erfolg, -errno im Fehlerfall
 */
int arch_vm86(uint8_t intr, void* regs, uint32_t* memory)
{
    // Wir koennen nur einen VM86-Task
    if (vm86_status.in_use) {
        return -EBUSY;
    }

    uint8_t* first_mb = mmc_valloc(&mmc_current_context(),
        (0xA0000 + PAGE_SIZE - 1) / PAGE_SIZE, PTE_P | PTE_W | PTE_U);

    memcpy(first_mb, &bios_data, 4096);

    // Videospeicher mappen
    mmc_map(&mmc_current_context(), first_mb + 0xA0000, (paddr_t) 0xA0000,
        PTE_U | PTE_W | PTE_P, (0x20000 + PAGE_SIZE - 1) / PAGE_SIZE);

    // BIOS mappen
    mmc_map(&mmc_current_context(), first_mb + 0xC0000, (paddr_t) 0xC0000,
        PTE_U | PTE_W | PTE_P, (0x40000 + PAGE_SIZE - 1) / PAGE_SIZE);

    // Speicherbereiche reinkopieren
    if (memory != NULL) {
        uint32_t infosize = memory[0];
        uint32_t i;

        for (i = 0; i < infosize; i++) {
            uint32_t addr = memory[1 + i * 3];
            uint32_t src = memory[1 + i * 3 + 1];
            uint32_t size = memory[1 + i * 3 + 2];

            memcpy(first_mb + addr, (void*) src, size);
        }
    }

    // Informationen speichern
    // TODO Ordentliches Locking fuer SMP
    vm86_status.in_use   = 1;
    vm86_status.caller   = current_thread;
    vm86_status.first_mb = first_mb;
    vm86_status.memory   = memory;
    vm86_status.regs     = regs;

    // Innerhalb vom VM86 einen RPC-Handler aufzurufen waere unklug
    pm_block_rpc(current_process, current_process->pid);

    // Task erstellen
    create_vm86_task(intr, regs, 0x9FC00);

    return 0;
}

/**
 * Beendet einen VM86-Task, kopiert alle zu zurueckzugebenden Daten und setzt
 * die Ausfuehrung des aufrufenden Tasks fort.
 */
static void destroy_vm86_task(cpu_tss_t* tss)
{
    pm_thread_t* vm86_task = current_thread;

    // Den Thread loeschen und den Aufrufer wieder aktiv machen
    current_thread = vm86_status.caller;
    if (current_thread->status != PM_STATUS_BLOCKED) {
        panic("VM86-Aufrufer ist nicht mehr blockiert!");
    }
    current_thread->status = PM_STATUS_RUNNING;
    vm86_task->status = PM_STATUS_BLOCKED;
    pm_thread_destroy(vm86_task);

    if (vm86_status.memory != NULL) {
        uint32_t infosize = vm86_status.memory[0];
        uint32_t i;

        for (i = 0; i < infosize; i++) {
            uint32_t addr = vm86_status.memory[1 + i * 3];
            uint32_t src = vm86_status.memory[1 + i * 3 + 1];
            uint32_t size = vm86_status.memory[1 + i * 3 + 2];

            memcpy((void*) src, vm86_status.first_mb + addr, size);
        }
    }

    mmc_vfree(&mmc_current_context(), vm86_status.first_mb,
        (0xA0000 + PAGE_SIZE - 1) / PAGE_SIZE);
    mmc_unmap(&mmc_current_context(), vm86_status.first_mb + 0xA0000,
        (0x60000 + PAGE_SIZE - 1) / PAGE_SIZE);

    // Register sichern
    vm86_regs_t* regs = vm86_status.regs;
    regs->ax = tss->eax;
    regs->bx = tss->ebx;
    regs->cx = tss->ecx;
    regs->dx = tss->edx;
    regs->si = tss->esi;
    regs->di = tss->edi;
    regs->ds = tss->ds;
    regs->es = tss->es;

    mmc_destroy(&vm86_status.mmc);

    // Wir sind fertig mit VM86 :-)
    pm_unblock_rpc(current_process, current_process->pid);
    vm86_status.in_use = 0;
}

/** Pusht einen Wert auf den Stack des VM86-Tasks */
static inline void emulator_push(cpu_tss_t* tss, uint16_t value)
{
    tss->esp -= 2;
    ((uint16_t*)(vm86_status.first_mb + tss->esp + (tss->ss << 4)))[0] = value;
}

/** Popt einen Wert vom Stack des VM86-Tasks */
static inline uint16_t emulator_pop(cpu_tss_t* tss)
{
    uint16_t res = ((uint16_t*)(vm86_status.first_mb + tss->esp + (tss->ss << 4)))[0];
    tss->esp += 2;
    return res;
}

/**
 * Diese Funktion wird vom Interrupthandler bei Exceptions aufgerufen, wenn der
 * aktuelle Task ein VM86-Task ist. Die Exception kann dann entweder hier
 * behandelt werden oder an die allgemeinen Exceptionhandlern weitergegeben
 * werden
 *
 * @return 1 wenn die Exception fertig behandelt ist; 0, wenn die normalen
 * Exceptionhandler aufgerufen werden sollen.
 */
int vm86_exception(interrupt_stack_frame_t* isf)
{
    // Bei #GP muessen wir was emulieren, ansonsten koennen wir fuer den Task
    // hier nichts tun
    if (isf->interrupt_number != 13) {
        return 0;
    }

    cpu_tss_t* tss = (cpu_tss_t*) isf->ebp;

    // Ein toller Emulator fuer privilegierte Instruktionen
    uint8_t* ops = (uint8_t*)(vm86_status.first_mb + tss->eip + (tss->cs << 4));
    uint16_t opcode;

    opcode = ops[0];
    if (opcode == 0x66) {
        opcode = 0x6600 | ops[1];
    }

    switch (opcode) {

        case 0x9c: /* pushf */
            emulator_push(tss, tss->eflags);
            tss->eip++;
            break;

        case 0x9d: /* popf */
            // So tun, als würden wir die EFLAGS wiederherstellen.
            // Das hier ist wohl alles andere als korrekt, aber funzt erstmal.
            emulator_pop(tss);
            tss->eip++;
            break;

        case 0xcd: /* int */
        {
            uint16_t intr = ops[1] & 0xff;
            uint16_t* ivt_entry = bios_data.ivt[intr];

            emulator_push(tss, tss->eip + 2);
            emulator_push(tss, tss->cs);
            emulator_push(tss, tss->eflags);

            tss->eip = ivt_entry[0];
            tss->cs  = ivt_entry[1];
            break;
        }

        case 0xcf: /* iret */

            // Wenn es das finale iret ist, koennen wir den VM86-Task beenden
            if (tss->esp == 0xFFFE) {
                destroy_vm86_task(tss);
                return 1;
            }

            // Ansonsten muss es ganz normal emuliert werden
            emulator_pop(tss);
            tss->cs  = emulator_pop(tss);
            tss->eip = emulator_pop(tss);
            break;

        case 0xec: /* inb al, dx */
            tss->eax &= ~0xFF;
            tss->eax |= inb(tss->edx);
            tss->eip++;
            break;

        case 0xed: /* inw ax, dx */
            tss->eax &= ~0xFFFF;
            tss->eax |= inw(tss->edx);
            tss->eip++;
            break;

        case 0x66ed: /* inl eax, dx */
            tss->eax = inl(tss->edx);
            tss->eip++;
            break;

        case 0xee: /* outb dx, al */
            outb(tss->edx, tss->eax);
            tss->eip++;
            break;

        case 0xef: /* outw dx, ax */
            outw(tss->edx, tss->eax);
            tss->eip++;
            break;

        case 0x66ef: /* outl dx, eax */
            outl(tss->edx, tss->eax);
            tss->eip++;
            break;

        default:
            kprintf("vm86: Unbekannte Opcodesequenz %02x %02x %02x %02x %02x "
                "%02x\n", ops[0], ops[1], ops[2], ops[3], ops[4], ops[5]);

            // Für ordentliches Rot
            isf->eflags = tss->eflags;
            isf->eip = tss->eip;
            isf->esp = tss->esp;
            isf->cs = tss->cs;
            isf->ss = tss->cs;

            return 0;
    }

    return 1;
}
