/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
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

#include <types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "kernel.h"
#include "syscall.h"
#include "mm.h"
#include "tasks.h"
#include "cpu.h"
#include "im.h"

typedef struct {
    uintptr_t   old_eip;
    uintptr_t   old_esp;

    // FIXME Das tut nur fuer i386...
    uint32_t       eflags;
    uint32_t       eax;

    uint8_t     reenable_irq;

    pm_thread_t* caller;
} rpc_t;

/**
 * RPC-Handler registrieren. Dieser wird bei jedem RPC aufgerufen und verteilt
 * die RPCs Prozessintern weiter.
 *
 * @param address Adresse des RPC-Handlers
 */
void syscall_set_rpc_handler(vaddr_t address)
{
   current_process->rpc_handler = address;
}


#if CONFIG_ARCH == ARCH_I386
void increase_user_stack_size(pm_thread_t* task_ptr, int pages);

static void adjust_stack_size(pm_thread_t* callee,
    interrupt_stack_frame_t* callee_isf, size_t rounded_data_size)
{
    // Falls der Stack nicht ausreicht, vergrößern
    size_t pages_count = ((rounded_data_size + PAGE_SIZE - 1) / PAGE_SIZE);
    if ((rounded_data_size % PAGE_SIZE) > (callee_isf->esp % PAGE_SIZE)) {
        pages_count++;
    }

	if(mmc_resolve(&callee->process->context,
        (vaddr_t) callee_isf->esp - rounded_data_size) == (paddr_t) NULL)
	{
        // TODO Das könnte bis zu (pages_count - 1) Pages zu viel reservieren
		increase_user_stack_size(callee, pages_count);
	}
}

/**
 * Führt einen RPC durch.
 *
 * Der Aufruf einer RPC-Funktion besteht im allgemeinen aus Metadaten (z.B.
 * die Funktionsnummer der aufzurufenden Funktion, Nachrichten-ID usw.)
 * und Nutzdaten.
 *
 * Die Daten werden dem aufgerufenen Task so auf den Stack gelegt, daß von
 * esp bis esp + metadata_size die Metadaten liegen, von esp + metadata_size
 * bis esp + metadata_size + data_size die Nutzdaten. Daraus ergibt sich
 * insbesondere, daß sich die Grenze zwischen Metadaten und Daten beliebig
 * ziehen läßt, wie sie für den Aufrufer am günstigsten ist. Unter anderem
 * ist es möglich, alle Daten an einem Ort zu übergeben und metadata_size = 0
 * zu setzen.
 *
 * Im Allgemeinen liegen allerdings die Metadaten auf dem Stack des Aufrufers,
 * während die Nutzdaten oft ein Pointer in den Heap des Aufrufers sind.
 *
 * Zur Durchführung eines RPC werden zunächst einige Register des
 * aufgerufenen Tasks gesichtert (eax, eip, esp, eflags; die übrigen Register
 * hat der aufgerufene Task selbst zu sichern). Anschließend werden die Daten
 * auf den Stack des aufgerufenen Tasks gelegt, eip auf den RPc-Handler gesetzt
 * und der Scheduler angewiesen, zum aufgerufenen Task umzuschalten.
 *
 * Der RPC-Handler muß als letzte Aktion den Syscall FASTRPC_RET aufrufen, der
 * die Register wiederherstellt und dadurch den Rücksprung vornimmt.
 *
 * TODO Aktualisieren
 * @param callee        Aufzurufender Task
 * @param metadata_size Länge der Metadaten in Bytes
 * @param metadata      Pointer auf die Metadaten
 * @param data_size     Länger der Daten in Bytes
 * @param data          Pointer auf die Daten
 * @param syscall       true wenn es sich um einen IRQ handelt, false sonst
 *
 * @return true, wenn der Aufruf erfolgreich durchgeführt wurde, false
 * sonst. Bei false als Rückgabe sollte der aufrufende Task den RPC-Syscall
 * wiederholen.
 */
int do_fastrpc(pid_t callee_pid, size_t metadata_size, void* metadata,
    size_t data_size, void* data, bool syscall)
{
    pm_process_t* process = pm_get(callee_pid);
    pm_thread_t* callee = NULL;

    if (!process) {
        // FIXME
        // abort_task("RPC zu nicht vorhandenem Prozess");
        return -ESRCH;
    }

    int i = 0;
    while ((callee = list_get_element_at(process->threads, i++)) &&
           callee->status != PM_STATUS_WAIT_FOR_RPC);
    if (!callee) {
        callee = list_get_element_at(process->threads, 0);
    }
    if (!callee) {
        panic("Prozess ohne Threads");
    }

    if (metadata_size + data_size > 8 * PAGE_SIZE) {
        // FIXME
        // abort_task("RPC-Datenblock zu gross: %d Bytes", data_size);
        return -EINVAL;
    }

    if (callee->process->rpc_handler == NULL) {
        return -EINVAL;
    }

/*
    if (callee == current_thread) {
        kprintf("PID %d: Self-RPC ignoriert\n", callee->process->pid);
        return 0;
    }
*/

    // TODO Pointer validieren (is_userspace)

    // Gesamtgröße, um die der Stack des aufgerufenen Prozesses vergrößert
    // wird (zusätzlich 4 Bytes für die Rücksprungadresse)
    size_t total_data_size = metadata_size + data_size
        + sizeof(data_size) + sizeof(pid_t);

    // Datengröße auf ganze vier Bytes aufrunden
    size_t rounded_data_size = (total_data_size + 3) & ~0x3;

    // Wenn es sich um eine Antwort im Rahmen von asynchronem RPC handelt,
    // durchlassen, auch wenn der Prozess nicht blockiert werden kann
    bool ignore_blocked = false;
    if ((callee->status == PM_STATUS_WAIT_FOR_RPC) && (
                ((metadata_size >= 4)
            &&  (((uint32_t*)metadata)[0] == 513))

        ||      ((metadata_size == 0)
            &&  (data_size >= 4)
            &&  (((uint32_t*)data)[0] == 513))
        ))
    {
        ignore_blocked = true;
    }

    // Wenn der Task nicht blockiert werden kann, soll der Aufrufer es später
    // nochmal versuchen.
    if (!ignore_blocked && callee->process->blocked_by_pid) {
        goto fail_retry;
    }

    // Wenn sich der Task gerade in einem Kernel-Int befindet, lassen wir ihn
    // besser in Ruhe und probieren spaeter nochmal.
    if ((current_thread != callee) &&
        (callee->kernel_stack != callee->user_isf))
    {
        return -EAGAIN;
    }

    // RPC-Backlinkinformation anlegen
    interrupt_stack_frame_t* callee_isf = callee->user_isf;
    rpc_t* rpc = malloc(sizeof(rpc_t));

    rpc->old_eip = callee_isf->eip;
    rpc->old_esp = callee_isf->esp;
    rpc->eax     = callee_isf->eax;
    rpc->eflags  = callee_isf->eflags;
    rpc->caller  = current_thread;

    if ((callee == current_thread) && syscall) {
        // Bei einem Self-RPC, der ein Syscall war (kein kernelinterner Aufruf
        // wie z.B. fuer  IRQ oder Timer), muss hinterher der Rueckgabewert
        // des RPC-Syscalls in eax stehen. Ein IRQ erscheint auch
        // als Self-RPC, darf aber eax nicht kaputt machen, da dort kein
        // Rueckgabewert erwartet wird, sondern der vorherige Wert.
        rpc->eax = 0;
    }

    rpc->reenable_irq = 0;

    list_push(callee->process->rpcs, rpc);


    // Stack des aufgerufenen Prozesses modifizieren:
    //
    // * Stack vergroessern, falls nicht genug freier Platz vorhanden ist
    // * Kopieren der Datengroesse, der Aufrufer-PID und der Daten auf den
    //   Stack des aufgerufenen Tasks und Anpassen des Stackpointers
    //
    // Dazu wird der Stack des aufgerufenen Tasks temporaer gemappt

    adjust_stack_size(callee, callee_isf, rounded_data_size);
    callee_isf->esp -= rounded_data_size;

    void* first_stack_page = mmc_automap_user(
        &mmc_current_context(),
        &callee->process->context,
        (vaddr_t) PAGE_ALIGN_ROUND_DOWN(callee_isf->esp),
        NUM_PAGES(rounded_data_size + ((uintptr_t) callee_isf->esp % PAGE_SIZE)),
        KERNEL_MEM_START, KERNEL_MEM_END,
        MM_FLAGS_KERNEL_DATA);
    void* stack = first_stack_page + ((uintptr_t) callee_isf->esp % PAGE_SIZE);

    ((uint32_t*) stack)[0] = data_size + metadata_size;
    ((uint32_t*) stack)[1] = current_process->pid;

#if 0
    kprintf("RPC PID %d => %d\n",
        current_process->pid,
        callee->process->pid);
    kprintf("Kopiere: %08x => %08x, %d Bytes\n",
        metadata, stack + 8, metadata_size);
    memcpy(stack + 8, metadata, metadata_size);
    kprintf("Kopiere: %08x => %08x, %d Bytes\n",
        data, stack + 8 + metadata_size, data_size);
#endif
    memcpy(stack + 8, metadata, metadata_size);
    memcpy(stack + 8 + metadata_size, data, data_size);


    // Zum RPC-Handler springen
    callee_isf->eip = (uintptr_t) callee->process->rpc_handler;

    // Temporaere Mappings wieder aufheben
    mmc_unmap(&mmc_current_context(), first_stack_page,
        NUM_PAGES(rounded_data_size));

//    kprintf("[%d => %d] RPC durchgefuehrt.\n",
//        current_process->pid,
//        callee->process->pid);

    // Der aufgerufene Task darf wieder laufen
    if (callee->status == PM_STATUS_WAIT_FOR_RPC ||
        callee->status == PM_STATUS_WAIT_FOR_LIO)
    {
        callee->status = PM_STATUS_READY;
    }

    // Zum aufgerufenen Task wechseln
    pm_scheduler_try_switch(callee);

    return 0;

fail_retry:
    // Jetzt direkt nochmal probieren ist nicht sinnvoll, lassen wir erstmal
    // einen anderen Task vor
    pm_scheduler_push(current_thread);
    current_thread = pm_scheduler_pop();
    return -EAGAIN;
}

int syscall_fastrpc(pid_t callee_pid, size_t metadata_size, void* metadata,
    size_t data_size, void* data)
{
    return do_fastrpc(callee_pid, metadata_size, metadata, data_size, data,
        true);
}

/**
 * Führt einen RPC zur Behandlung eines IRQ durch. Der IRQ wird dabei
 * deaktiviert, solange der Handler arbeitet und erst anschließend
 * wieder freigegeben. Im sonstigen Verhalten gleicht diese Funktion
 * fastrpc.
 *
 * @see fastrpc
 */
bool fastrpc_irq(pm_process_t* callee, size_t metadata_size, void* metadata,
    size_t data_size, void* data, uint8_t irq)
{
    int ret;

    if (callee->blocked_by_pid && (callee->status != PM_STATUS_WAIT_FOR_RPC)) {
        return false;
    }

    ret = do_fastrpc(
        callee->pid, metadata_size, metadata, data_size, data, false);

    if (ret < 0) {
        return false;
    }

    im_irq_handlers_increment(irq);

    rpc_t* rpc = list_get_element_at(callee->rpcs, 0);
    rpc->reenable_irq = irq;

    return true;
}

/**
 * Wird nach der Ausführung eines RPC-Handlers aufgerufen.
 *
 * Nach der Rückkehr vom RPC-Handler wird der neueste Zustand vom RPC-Stack
 * gepopt und zur Wiederherstellung des ursprünglichen Prozessorzustands
 * benutzt.
 *
 * Dies betrifft eip, esp, eax und eflags. Die übrigen Register sind vom
 * RPC-Handler zu sichern und vor dem Aufruf von SYSCALL_FASTRPC_RET
 * wiederherzustellen. eax ist davon ausgenommen, da es die Funktionsnummer
 * des Syscalls enthalten muß.
 *
 * @param esp Interrupt-Stackframe des vom RPC-Handler zurückkehrenden Tasks
 */
void syscall_fastrpc_ret(void)
{
    rpc_t* rpc = list_pop(current_process->rpcs);
    interrupt_stack_frame_t* callee_isf = current_thread->user_isf;

    // Wenn der Task vom RPC-Handler zurückkehrt, obwohl der Handler
    // gar nicht aufgerufen wurde, läuft was schief
    if (rpc == NULL) {
#if 0
        if(debug_test_flag(DEBUG_FLAG_STACK_BACKTRACE)) {
            stack_backtrace_ebp(callee_isf->ebp, callee_isf->eip);
        }
#endif
        panic("Unerwartete Rueckkehr vom RPC-Handler\n");
        // TODO abort_task("Unerwartete Rueckkehr vom RPC-Handler");
        return;
    }

    // Wiederherstellen der Register
    callee_isf->eip = rpc->old_eip;
    callee_isf->esp = rpc->old_esp;
    callee_isf->eax = rpc->eax;
    callee_isf->eflags = rpc->eflags;

//    kprintf("[%d => %d] RPC fertig.\n",
//        rpc->caller->process->pid,
//        current_process->pid);

    // Wechsel zum aufrufenden Task
    if (rpc->caller) {
        pm_scheduler_try_switch(rpc->caller);
    }

    // Wenn es ein IRQ-verarbeitender RPC war, den Interrupt jetzt
    // wieder aktivieren
    if (rpc->reenable_irq) {
        im_irq_handlers_decrement(rpc->reenable_irq);
    }

    free(rpc);
}

/**
 * Entfernt den gegebenen Task aus allen RPC-Backlinks.
 *
 * Diese Funktion wird benoetigt, wenn ein Task einen RPC aufruft und beendet
 * wird, bevor der RPC fertig ist. Ansonsten zeigt rpc->caller ins Leere,
 * was in return_from_rpc() zur Katastrophe führt.
 */
void rpc_destroy_task_backlinks(pm_process_t* destroyed_process)
{
    pm_process_t* proc;
    int i;

    for (i = 0; (proc = list_get_element_at(process_list, i)); i++) {
        int j;
        rpc_t* rpc;
        for (j = 0; (rpc = list_get_element_at(proc->rpcs, j)); j++) {
            if (rpc->caller && (rpc->caller->process == destroyed_process)) {
                rpc->caller = NULL;
            }
        }
    }
}

void syscall_add_interrupt_handler(uint32_t intr)
{
    im_add_handler(intr, current_process);
}

#endif
