/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
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

#include <types.h>
#include <stdint.h>
#include <string.h>

#include "syscall.h"
#include "tasks.h"
#include "cpu.h"

/**
 * Neuen Prozess mit einem Thread erstellen. Der neue Task wird blockiert, bis
 * er vom Ersteller entblockt wird.
 *
 * @param start Start-Adresse
 * @param uid User-Id
 * @param cmdline Kommandozeile mit der der Prozess gestartet wurde
 * @param parent_pid PID des Elternprozesses
 *
 * @return PID des neuen Tasks
 */
pid_t syscall_pm_create_process(vaddr_t start, uid_t uid, const char* cmdline,
    pid_t parent_pid)
{
    // Eltern-Prozess holen
    pm_process_t* parent = pm_get(parent_pid);
    // TODO: UID?
    pm_process_t* process = pm_create(parent, cmdline);
    pm_thread_create(process, start);
//    kprintf("pm_create_process %d\n", process->pid);
    return process->pid;
}

/**
 * Beendet den aktuellen Prozess.
 */
void syscall_pm_exit_process(void)
{
    pm_process_t* old_task = cpu_get_current()->thread->process;
    pm_thread_t* thread;

    // Task abgeben
    pm_scheduler_push(cpu_get_current()->thread);

    // Genau jetzt muss der Prozess geloescht werden: Der Thread ist nicht mehr
    // aktiv, und er darf auch nicht wieder aktiviert werden.
    pm_destroy(old_task);

    // Einen neuen Task holen.
    // FIXME Da braucht es endlich eine Funktion dafuer
    thread = pm_scheduler_pop();

    cpu_get_current()->tss.esp0 = (uintptr_t) thread->kernel_stack + sizeof(interrupt_stack_frame_t);
    cpu_get_current()->tss.ss0 = 2 << 3;

    cpu_get_current()->thread = thread;
}

/**
 * PID des aktuellen Prozesses ausfindig machen
 *
 * @return PID
 */
pid_t syscall_pm_get_pid()
{
    return cpu_get_current()->thread->process->pid;
}

/**
 * PID des Elternprozesses ausfindig machen
 *
 * @param pid PID des Prozesses
 *
 * @return PID
 */
pid_t syscall_pm_get_parent_pid(pid_t pid)
{
    pm_process_t* process;

    if (pid == 0) {
        process = cpu_get_current()->thread->process;
    } else {
        process = pm_get(pid);
    }

    if ((process == NULL) || (process->parent == NULL)) {
        return 0;
    }

    return process->parent->pid;
}

/**
 * Befehlszeile des aktuellen Prozesses abfragen
 *
 * @return Pointer auf die Befehlszeile
 */
const char* syscall_pm_get_cmdline()
{
    const char* cmdline = cpu_get_current()->thread->process->cmdline;
    if (cpu_get_current()->thread->process->cmdline == NULL) {
        return NULL;
    }
    size_t cmdlinesize = strlen(cmdline) + 1;
    vaddr_t address = mmc_automap(&mmc_current_context(), pmm_alloc(NUM_PAGES(
        cmdlinesize)), NUM_PAGES(cmdlinesize), USER_MEM_START, USER_MEM_END,
        MM_FLAGS_USER_DATA);
    memcpy(address, cmdline, cmdlinesize);

    return (const char*) address;
}

/**
 * Einen Speicherbereich vom aktuellen an einen anderen Prozess uebertragen.
 * Die Pages werden aus dem Adressraum des aktuellen Prozess entfernt. Dieser
 * Syscall darf nur benutzt werden, solange der Prozess noch nicht gestartet
 * wurde. Falls der Bereich schon gemappt ist im Zielprozess wird er
 * ueberschrieben. Dabei muss beachtet werden, dass immer ganze Pages kopiert
 * oder verschoben werden!
 *
 * @param pid PID des Prozess an den die Page uebertragen werden soll
 * @param src Adresse im Adressraum des aktuellen Prozesses
 * @param dest Adresse im Adressraum des Zielprozesses
 * @param size Groesse des Speicherbereichs.
 */
void syscall_init_child_page(pid_t pid, vaddr_t dest, vaddr_t src, size_t size)
{
    //kprintf("init child_page %d  0x%08x  0x%08x Bytes\n", pid, dest, size);
    pm_process_t* process = pm_get(pid);
    if (process == NULL) {
        return;
    }

    // TODO Im Kommentar genannte Bedingungen und Berechtigungen pruefen

    // Adressen abrunden auf ein Vielfaches von PAGE_SIZE
    src = (vaddr_t) PAGE_ALIGN_ROUND_DOWN((uintptr_t) src);
    dest = (vaddr_t) PAGE_ALIGN_ROUND_DOWN((uintptr_t) dest);

    // Anzahl der Seiten berechnen
    size_t num_pages = NUM_PAGES(size);

    // Adressen kopieren damit sie veraendert werden koennen
    // Sie duerfen nicht ueberschrieben werden, weil sie am Schluss noch
    // gebraucht werden, um den Speicher aus dem Quellprozess zu entfernen
    vaddr_t src_cur = src;
    vaddr_t dest_cur = dest;
    while (num_pages-- != 0) {
        // Wenn die Page schon gemappt ist, wird der notwendige Bereich kopiert
        // sonst wird die Page nur umgemappt
        paddr_t dest_phys = mmc_resolve(&process->context, dest_cur);
        paddr_t src_phys = mmc_resolve(&mmc_current_context(), src_cur);

        if (dest_phys != NULL) {
            // Die Page temporaer mappen
            vaddr_t dest_vaddr = vmm_kernel_automap(dest_phys, PAGE_SIZE);
            memcpy(dest_vaddr, src_cur, PAGE_SIZE);
            vmm_kernel_unmap(dest_vaddr, PAGE_SIZE);
        } else {
            // Page in den anderen Prozess mappen
            mmc_map(&process->context, dest_cur, src_phys, MM_FLAGS_USER_DATA,
                1);
        }
        // Adressen erhoehen
        src_cur = (vaddr_t) ((uintptr_t) src_cur + PAGE_SIZE);
        dest_cur = (vaddr_t) ((uintptr_t) dest_cur + PAGE_SIZE);
    }

    // Speicherbereich aus dem Adressraum des Quellprozesses entfernen
    mmc_unmap(&mmc_current_context(), src, NUM_PAGES(size));
}

/**
 * Die Kontrolle an einen anderen Task abgeben
 *
 * FIXME Diesen Code gibt es in genau dieser Form auch in im.c
 */
void syscall_pm_sleep(void)
{
    pm_thread_t* thread = cpu_get_current()->thread;

    // Den aktuellen Thread an den Scheduler zurueckgeben
    pm_scheduler_push(thread);

    // Einen neuen Thread holen.
    thread = pm_scheduler_pop();

    cpu_get_current()->tss.esp0 = (uintptr_t) thread->kernel_stack + sizeof(interrupt_stack_frame_t);
    cpu_get_current()->tss.ss0 = 2 << 3;

    cpu_get_current()->thread = thread;
}

/**
 * Die Kontrolle an einen anderen Task abgeben und erst wieder aufwachen,
 * wenn ein RPC zu bearbeiten ist
 */
void syscall_pm_wait_for_rpc(void)
{
    pm_thread_t* thread = cpu_get_current()->thread;
    syscall_pm_sleep();
    thread->status = PM_STATUS_WAIT_FOR_RPC;
}
