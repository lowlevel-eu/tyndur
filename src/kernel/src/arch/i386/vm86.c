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
    uint32_t*       memory;
    pm_thread_t*    caller;
    vm86_regs_t*    regs;
} vm86_status = {
    .in_use     =   false,
};

/*
 * Ein VM86-Segment ist 4k groß (0x10000). Wir dürfen sp aber nicht ganz oben
 * hinsetzen, weil der Interrupthandler davon ausgehen darf, dass er per INT
 * aufgerufen wurde, und das legt sechs Bytes auf den Stack.
 *
 * SeaBIOS braucht das, weil es diese sechs Bytes wegsichert und später
 * wiederherstellt. Wenn wir ganz oben anfangen, wird das ss-Segementlimit
 * überschritten und wir bekommen eine #SS-Exception.
 */
#define INITIAL_SP 0xfffa

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
                                         (vaddr_t)(uintptr_t)ivt_entry[0]);

    // Na ja... Fast normal.
    task->vm86 = true;

    interrupt_stack_frame_t* isf = task->user_isf;
    struct vm86_isf* visf = (struct vm86_isf*)
        ((uintptr_t) task->kernel_stack_bottom +
        task->kernel_stack_size - sizeof(struct vm86_isf));
    task->kernel_stack = visf;
    task->user_isf = visf;

    memmove(&visf->isf, isf, sizeof(*isf));
    isf = &visf->isf;

    isf->eflags = 0x20202;
    isf->eax = regs->ax;
    isf->ebx = regs->bx;
    isf->ecx = regs->cx;
    isf->edx = regs->dx;
    isf->esi = regs->si;
    isf->edi = regs->di;
    isf->esp = INITIAL_SP;
    isf->ebp = 0;
    isf->cs = ivt_entry[1];
    isf->ss = (stack - 65536) >> 4;
    visf->ds = regs->ds;
    visf->es = regs->es;
    visf->fs = 0;
    visf->gs = 0;

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

    uint8_t* first_mb = mmc_find_free_pages(&mmc_current_context(),
        0x100000 / PAGE_SIZE, MM_USER_START, MM_USER_END);

    if (first_mb == NULL) {
        return -ENOMEM;
    }

    mmc_valloc_limits(&mmc_current_context(),
        0xA0000 / PAGE_SIZE, 0, 0,
        0, 0xA0000, PTE_P | PTE_W | PTE_U | PTE_ALLOW_NULL);

    memcpy(0, &bios_data, 4096);

    // Videospeicher und BIOS mappen
    mmc_map(&mmc_current_context(), (vaddr_t) 0xA0000, (paddr_t) 0xA0000,
        PTE_U | PTE_W | PTE_P, (0x60000 + PAGE_SIZE - 1) / PAGE_SIZE);

    // Speicherbereiche reinkopieren
    if (memory != NULL) {
        uint32_t infosize = memory[0];
        uint32_t i;

        for (i = 0; i < infosize; i++) {
            uint32_t addr = memory[1 + i * 3];
            uint32_t src = memory[1 + i * 3 + 1];
            uint32_t size = memory[1 + i * 3 + 2];

            if (addr >= 0xA0000) {
                return -EFAULT;
            }

            memcpy((void*) addr, (void*) src, size);
        }
    }

    // Informationen speichern
    // TODO Ordentliches Locking fuer SMP
    vm86_status.in_use   = 1;
    vm86_status.caller   = current_thread;
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
static void destroy_vm86_task(interrupt_stack_frame_t* isf)
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

            memcpy((void*) src, (void*) addr, size);
        }
    }

    mmc_vfree(&mmc_current_context(), (vaddr_t) 0,
        (0xA0000 + PAGE_SIZE - 1) / PAGE_SIZE);
    mmc_unmap(&mmc_current_context(), (vaddr_t) 0xA0000,
        (0x60000 + PAGE_SIZE - 1) / PAGE_SIZE);

    // Register sichern
    vm86_regs_t* regs = vm86_status.regs;
    regs->ax = isf->eax;
    regs->bx = isf->ebx;
    regs->cx = isf->ecx;
    regs->dx = isf->edx;
    regs->si = isf->esi;
    regs->di = isf->edi;
    regs->ds = isf->ds;
    regs->es = isf->es;

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
    uint16_t opcode;

    opcode = ops[0];
    if (opcode == 0x66) {
        opcode = 0x6600 | ops[1];
    }

    switch (opcode) {

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

            emulator_push(isf, isf->eflags);
            emulator_push(isf, isf->cs);
            emulator_push(isf, isf->eip + 2);

            isf->eip = ivt_entry[0];
            isf->cs  = ivt_entry[1];
            break;
        }

        case 0xcf: /* iret */

            // Wenn es das finale iret ist, koennen wir den VM86-Task beenden
            if (isf->esp == INITIAL_SP) {
                destroy_vm86_task(isf);
                return 1;
            }

            // Ansonsten muss es ganz normal emuliert werden
            isf->eip = emulator_pop(isf);
            isf->cs  = emulator_pop(isf);
            emulator_pop(isf);
            break;

        case 0xe4: /* in al, imm8 */
            isf->eax &= ~0xFF;
            isf->eax |= inb(ops[1]);
            isf->eip += 2;
            break;

        case 0xe5: /* in ax, imm8 */
            isf->eax &= ~0xFFFF;
            isf->eax |= inw(ops[1]);
            isf->eip += 2;
            break;

        case 0x66e5: /* in eax, imm8 */
            isf->eax = inl(ops[2]);
            isf->eip += 3;
            break;

        case 0xe6: /* out imm8, al */
            outb(ops[1], isf->eax);
            isf->eip += 2;
            break;

        case 0xe7: /* out imm8, ax */
            outw(ops[1], isf->eax);
            isf->eip += 2;
            break;

        case 0x66e7: /* outl imm8, eax */
            outl(ops[2], isf->eax);
            isf->eip += 3;
            break;

        case 0xec: /* inb al, dx */
            isf->eax &= ~0xFF;
            isf->eax |= inb(isf->edx);
            isf->eip++;
            break;

        case 0xed: /* inw ax, dx */
            isf->eax &= ~0xFFFF;
            isf->eax |= inw(isf->edx);
            isf->eip++;
            break;

        case 0x66ed: /* inl eax, dx */
            isf->eax = inl(isf->edx);
            isf->eip += 2;
            break;

        case 0xee: /* outb dx, al */
            outb(isf->edx, isf->eax);
            isf->eip++;
            break;

        case 0xef: /* outw dx, ax */
            outw(isf->edx, isf->eax);
            isf->eip++;
            break;

        case 0x66ef: /* outl dx, eax */
            outl(isf->edx, isf->eax);
            isf->eip += 2;
            break;

        case 0xfa: /* cli */
            // Hoffentlich hatte der VM86-Code keinen guten Grund dafür, aber
            // Interrupts wirklich ausschalten klingt gefährlich
            isf->eip++;
            break;

        case 0xfb: /* sti */
            isf->eip++;
            break;

        default:
            kprintf("vm86: Unbekannte Opcodesequenz %02x %02x %02x %02x %02x "
                "%02x\n", ops[0], ops[1], ops[2], ops[3], ops[4], ops[5]);
            return 0;
    }

    return 1;
}
