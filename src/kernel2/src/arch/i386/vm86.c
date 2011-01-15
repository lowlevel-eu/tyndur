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
#include <string.h>
#include <syscall_structs.h>
#include <errno.h>

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
    void*           stack;
    paddr_t         nullpage;
    uint32_t*       memory;
    pm_thread_t*    caller;
    vm86_regs_t*    regs;
} vm86_status = {
    .in_use     =   false,
};

/**
 * Speichert BIOS-Daten, um sie den VM86-Tasks später bereitstellen zu können
 */
void vm86_init(void)
{
    memcpy(&bios_data, 0, 4096);
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
        (vaddr_t)(uintptr_t) ivt_entry[0]);

    interrupt_stack_frame_t* isf = task->kernel_stack;

    struct vm86_isf visf;


    // Register setzen
    isf->eflags = 0x20202;
    isf->cs = ivt_entry[1];
    isf->ss = stack / 16;

    isf->eax = regs->ax;
    isf->ebx = regs->bx;
    isf->ecx = regs->cx;
    isf->edx = regs->dx;
    isf->esi = regs->si;
    isf->edi = regs->di;
    isf->esp = PAGE_SIZE - 16;
    isf->ebp = 0;

    // Segmentregister setzen
    // Dabei wird der bisherige Kernelstack in die temporaere
    // visf-Datenstruktur kopiert, denn er muss verschoben werden, damit die
    // VM86-Segmentregister noch Platz auf dem Stack haben
    visf.isf = *isf;
    visf.ds = regs->ds;
    visf.es = regs->es;

    // Und das ganze auf den Stack
    task->kernel_stack = ((uint8_t*) task->kernel_stack)
        + sizeof(*isf) - sizeof(visf);
    task->user_isf = task->kernel_stack;
    memcpy(task->kernel_stack, &visf, sizeof(visf));

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
    int res = 0;

    // Wir koennen nur einen VM86-Task
    if (vm86_status.in_use) {
        return -EBUSY;
    }

    // Nullpage mappen
    uint32_t* page_table = (uint32_t*) PAGETABLES_MEM_START;
    paddr_t nullpage = pmm_alloc(1);
    page_table[0] = (uintptr_t) nullpage | PTE_U | PTE_W | PTE_P;
    asm volatile("invlpg %0" :: "m" (*(char*)0x0));
    memcpy(0, &bios_data, 4096);

    // Stack mappen
    void* stack = mmc_valloc_limits(&mmc_current_context(), 1, 0, 0,
        0x0, 0x9fc00, PTE_U | PTE_W | PTE_P);
    if (stack == NULL) {
        kprintf("vm86: Kein Platz fuer den Stack\n");
        res = -ENOMEM;
        goto fail_stack;
    }

    // Videospeicher mappen
    // FIXME Das tut nur durch Zufall
    bool ret = mmc_map(&mmc_current_context(),
        (vaddr_t) 0xa0000, (paddr_t) 0xa0000,
        PTE_U | PTE_W | PTE_P, 0x10);
    if (!ret) {
        kprintf("vm86: Kein Platz fuer den Videospeicher\n");
        res = -ENOMEM;
        goto fail_video;
    }

    // Falls noetig, ein paar Pages mehr mappen
    if (memory != NULL) {
        uint32_t infosize = memory[0];
        uint32_t i;

        for (i = 0; i < infosize; i++) {
            uint32_t addr = memory[1 + i * 3];
            uint32_t src = memory[1 + i * 3 + 1];
            uint32_t size = memory[1 + i * 3 + 2];

            if (size > PAGE_SIZE) {
                res = -EINVAL;
                goto fail_memory;
            }

            paddr_t phys_mem = pmm_alloc(1);
            if (!mmc_map(&mmc_current_context(), (vaddr_t)(addr & ~0xFFF),
                phys_mem, PTE_W | PTE_P | PTE_U, 1))
            {
                res = -ENOMEM;
                goto fail_memory;
            }
            memcpy((void*)addr, (void*)src, size);
        }
    }

    // Informationen speichern
    // TODO Ordentliches Locking fuer SMP
    vm86_status.in_use   = 1;
    vm86_status.caller   = current_thread;
    vm86_status.stack    = stack;
    vm86_status.nullpage = nullpage;
    vm86_status.memory   = memory;
    vm86_status.regs     = regs;

    // Innerhalb vom VM86 einen RPC-Handler aufzurufen waere unklug
    pm_block_rpc(current_process, current_process->pid);

    // Task erstellen
    create_vm86_task(intr, regs, (uintptr_t) stack);

    return 0;

fail_memory:
    mmc_unmap(&mmc_current_context(), (vaddr_t) 0xa0000, 0x10);
fail_video:
    mmc_vfree(&mmc_current_context(), stack, 1);
fail_stack:
    pmm_free(nullpage, 1);
    return res;
}

/**
 * Beendet einen VM86-Task, kopiert alle zu zurueckzugebenden Daten und setzt
 * die Ausfuehrung des aufrufenden Tasks fort.
 */
static void destroy_vm86_task(interrupt_stack_frame_t* isf)
{
    pm_thread_t* vm86_task = current_thread;
    struct vm86_isf* visf = (struct vm86_isf*)
        (((char*) isf) + (sizeof(*isf) - sizeof(*visf)));

    // Den Thread loeschen und den Aufrufer wieder aktiv machen
    vm86_task->status = PM_STATUS_BLOCKED;
    pm_thread_destroy(vm86_task);

    current_thread = vm86_status.caller;
    current_thread->status = PM_STATUS_RUNNING;

    // Temporaere Mappings rueckgaengig machen
    mmc_unmap(&mmc_current_context(), (vaddr_t) 0xa0000, 0x10);
    mmc_unmap(&mmc_current_context(), vm86_status.stack, 1);

    uint32_t* page_table = (uint32_t*) PAGETABLES_MEM_START;
    page_table[0] = 0;
    asm volatile("invlpg %0" :: "m" (*(char*)0x0));

    pmm_free(vm86_status.nullpage, 1);

    // Vom Benutzer angeforderte Mappings rueckgaengig machen
    if (vm86_status.memory != NULL) {
        uint32_t infosize = vm86_status.memory[0];
        uint32_t i;

        for (i = 0; i < infosize; i++) {
            uint32_t addr = vm86_status.memory[1 + i * 3];
            uint32_t src = vm86_status.memory[1 + i * 3 + 1];
            uint32_t size = vm86_status.memory[1 + i * 3 + 2];

            memcpy((void*)src, (void*)addr, size);
            pmm_free(mmc_resolve(&mmc_current_context(),
                (vaddr_t) (addr & ~0xFFF)), 1);
            mmc_unmap(&mmc_current_context(), (vaddr_t)(addr & ~0xFFF), 1);
        }
    }

    // Register sichern
    vm86_regs_t* regs = vm86_status.regs;
    regs->ax = isf->eax;
    regs->bx = isf->ebx;
    regs->cx = isf->ecx;
    regs->dx = isf->edx;
    regs->si = isf->esi;
    regs->di = isf->edi;
    regs->ds = visf->ds;
    regs->es = visf->es;

    // Wir sind fertig mit VM86 :-)
    pm_unblock_rpc(current_process, current_process->pid);
    vm86_status.in_use = 0;
}

/** Pusht einen Wert auf den Stack des VM86-Tasks */
static inline void emulator_push(interrupt_stack_frame_t* isf, uint16_t value)
{
    isf->esp -= 2;
    ((uint16_t*)(isf->esp + (isf->ss << 4)))[0] = value;
}

/** Popt einen Wert vom Stack des VM86-Tasks */
static inline uint16_t emulator_pop(interrupt_stack_frame_t* isf)
{
    uint16_t res = ((uint16_t*)(isf->esp + (isf->ss << 4)))[0];
    isf->esp += 2;
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

    // Ein toller Emulator fuer privilegierte Instruktionen
    uint8_t* ops = (uint8_t*)(isf->eip + (isf->cs << 4));
    switch (ops[0]) {

        case 0x9c: /* pushf */
            emulator_push(isf, isf->eflags);
            isf->eip++;
            break;

        case 0x9d: /* popf */
            // So tun, als würden wir die EFLAGS wiederherstellen.
            // Das hier ist wohl alles andere als korrekt, aber funzt erstmal.
            emulator_pop(isf);
            isf->eip++;
            break;

        case 0xcd: /* int */
        {
            uint16_t intr = ops[1] & 0xff;
            uint16_t* ivt_entry = bios_data.ivt[intr];

            emulator_push(isf, isf->eip + 2);
            emulator_push(isf, isf->cs);
            emulator_push(isf, isf->eflags);

            isf->eip = ivt_entry[0];
            isf->cs  = ivt_entry[1];
            break;
        }

        case 0xcf: /* iret */

            // Wenn es das finale iret ist, koennen wir den VM86-Task beenden
            if (isf->esp == PAGE_SIZE - 16) {
                destroy_vm86_task(isf);
                return 1;
            }

            // Ansonsten muss es ganz normal emuliert werden
            emulator_pop(isf);
            isf->cs  = emulator_pop(isf);
            isf->eip = emulator_pop(isf);
            break;

        default:
            kprintf("vm86: Unbekannter Opcode %x\n", ops[0]);
            return 0;
    }

    return 1;
}
