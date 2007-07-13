/*  
 * Copyright (c) 2007 The LOST Project. All rights reserved.
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

#include "rpc.h"
#include "tasks.h"
#include "paging.h"
#include "vmm.h"
#include "kmm.h"
#include "intr.h"
#include "string.h"
#include "stdlib.h"
#include "debug.h"

typedef struct {
    dword   old_eip;
    dword   old_esp;
    dword   eflags;
    dword   eax;

    struct task* caller;
} rpc_t;

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
 * ziehen läßt, wie sie für den Aufrufer am günstigsten ist. Unter anderem ist
 * es möglich, alle Daten an einem Ort zu übergeben und metadata_size = 0 zu
 * setzen.
 *
 * Im Allgemeinen liegen allerdings die Metadaten auf dem Stack des Aufrufers,
 * während die Nutzdaten oft ein Pointer in den Heap des Aufrufers sind.
 *
 * Zur Durchführung eines RPC werden zunächst einige Register des aufgerufenen
 * Tasks gesichtert (eax, eip, esp, eflags; die übrigen Register hat der
 * aufgerufene Task selbst zu sichern). Anschließend werden die Daten auf den
 * Stack des aufgerufenen Tasks gelegt, eip auf den RPc-Handler gesetzt und der
 * Scheduler angewiesen, zum aufgerufenen Task umzuschalten.
 *
 * Der RPC-Handler muß als letzte Aktion den Syscall FASTRPC_RET aufrufen, der
 * die Register wiederherstellt und dadurch den Rücksprung vornimmt.
 *
 * @param callee        Aufzurufender Task
 * @param metadata_size Länge der Metadaten in Bytes
 * @param metadata      Pointer auf die Metadaten
 * @param data_size     Länger der Daten in Bytes
 * @param data          Pointer auf die Daten
 *
 * @return TRUE, wenn der Aufruf erfolgreich durchgeführt wurde, FALSE
 * sonst. Bei FALSE als Rückgabe sollte der aufrufende Task den RPC-Syscall
 * wiederholen.
 */
bool fastrpc(struct task * callee, dword metadata_size, void* metadata, 
    dword data_size, void* data) 
{
    int i;

    //kprintf("[%d => %d, 0x%x + 0x%x]\n", current_task->pid, callee->pid, 
    //    metadata_size, data_size);
    //kprintf("<%08x = %08x %08x>\n", metadata, 0, 0);
    
    if (!callee) {
        abort_task("RPC zu nicht vorhandenem Prozess");
        return FALSE;
    }

    if (data_size > 8 * PAGE_SIZE) {
        abort_task("RPC-Datenblock zu gross: %d Bytes", data_size);
        return FALSE;
    }
    
    if (callee->rpc_handler == NULL) {
        return FALSE;
    }

    // Gesamtgröße, um die der Stack des aufgerufenen Prozesses vergrößert
    // wird (zusätzlich 4 Bytes für die Rücksprungadresse)
    dword total_data_size = metadata_size + data_size 
        + sizeof(data_size) + sizeof(pid_t);
    
    // Datengröße auf ganze vier Bytes aufrunden
    dword rounded_data_size = (total_data_size + 3) & ~0x3;

    // Während wir am Stack rumbasteln, sollte niemand dazwischenfunken und
    // erst recht nicht der Task weiterlaufen.
    // Wenn der Task nicht blockiert werden kann, soll der Aufrufer es später
    // nochmal versuchen.
    if (!block_task (callee, current_task->pid)) {
        return FALSE;
    }
    
    // Falls der Stack nicht ausreicht, vergrößern
    paddr_t callee_isf_phys = 
        resolve_vaddr((page_directory_t) callee->cr3, (vaddr_t) callee->esp);

    struct int_stack_frame* callee_isf = (struct int_stack_frame*) 
        map_phys_addr(callee_isf_phys, sizeof(callee_isf));
    
    dword pages_count = ((rounded_data_size + PAGE_SIZE - 1) / PAGE_SIZE);
    if ((rounded_data_size % PAGE_SIZE) > (callee_isf->esp % PAGE_SIZE)) {
        pages_count++;
    }

	if(resolve_vaddr((page_directory_t) callee->cr3, 
        (vaddr_t) callee_isf->esp - rounded_data_size) == NULL)
	{
        // TODO Das könnte bis zu (pages_count - 1) Pages zu viel reservieren
		increase_user_stack_size(callee, pages_count);
	}
    
    // Den Stack des aufgerufenen Tasks in den Kernelspeicher mappen
    dword callee_esp_phys = (dword) resolve_vaddr(
        (page_directory_t) callee->cr3, (vaddr_t) callee_isf->esp);
   
    // TODO Funktion, um komplette Bereiche eines anderen Tasks zu mappen
    vaddr_t callee_esp_kernel_pages = 
        find_contiguous_kernel_pages(pages_count);

    for (i = 0; i < pages_count; i++) {
        map_page(
            kernel_page_directory, 
            (vaddr_t) ((dword) callee_esp_kernel_pages + (PAGE_SIZE * i)), 
            (paddr_t) PAGE_ALIGN_ROUND_DOWN((dword) resolve_vaddr(
                (page_directory_t) callee->cr3, 
                (vaddr_t) callee_isf->esp - (PAGE_SIZE * (pages_count - i - 1))
            )),
            PTE_P | PTE_W
        );
    }

    // RPC-Backlinkinformation anlegen
    rpc_t* rpc = malloc(sizeof(rpc_t));
    
    rpc->old_eip = callee_isf->eip;
    rpc->old_esp = callee_isf->esp;
    rpc->eax     = callee_isf->eax;
    rpc->eflags  = callee_isf->eflags;
    rpc->caller  = current_task;

    list_push(callee->rpcs, rpc);
    
    // Adresse des neuen Stackendes im Kernelspeicher berechnen. Diese Adresse
    // liegt irgendwo in der Mitte der ersten gemappten Stackpage. Das Offset
    // von der Pagegrenze ist also in der Rechnung zu berücksichtigen.
    //
    // Auf den Stack kommen neue Daten der Größe rounded_data_size.
    vaddr_t callee_esp = callee_esp_kernel_pages
        + (callee_esp_phys % PAGE_SIZE) + ((pages_count - 1) * PAGE_SIZE);

    callee_esp -= rounded_data_size;

    // Kopieren der Datengröße, der Aufrufer-PID und der Daten auf den Stack
    *((dword *) callee_esp) = data_size + metadata_size;
    *((dword *) (callee_esp + 4)) = current_task->pid;
    memcpy(callee_esp + 8, metadata, metadata_size);
    memcpy(callee_esp + 8 + metadata_size, data, data_size);

    // Den Stackpointer des aufgerufenen Tasks anpassen, so daß er auf den 
    // Anfang der kopierten Daten zeigt, und in den RPC-Handler springen
    callee_isf->esp -= rounded_data_size;
    callee_isf->eip = (dword) callee->rpc_handler;

    // Gemappte Seiten wieder freigeben
    for (i = 0; i < pages_count; i++) {
        unmap_page(kernel_page_directory, 
            (vaddr_t) ((dword) callee_esp_kernel_pages + (PAGE_SIZE * i)));
    }

    unmap_page(kernel_page_directory, 
        (vaddr_t) PAGE_ALIGN_ROUND_DOWN((dword) callee_isf));
   
    // Der Task darf wieder laufen
    unblock_task(callee, current_task->pid);

    return TRUE;
}

/**
 * Wird nach der Ausführung eines RPC-Handlers aufgerufen.
 *
 * Nach der Rückkehr vom RPC-Handler wird der neueste Zustand vom RPC-Stack
 * gepopt und zur Wiederherstellung des ursprünglichen Prozessorzustands
 * benutzt.
 *
 * Dies betrifft eip, esp, eax und eflags. Die übrigen Register sind vom
 * RPC-Handler zu sicher und vor dem Aufruf von SYSCALL_FASTRPC_RET
 * wiederherzustellen. eax ist davon ausgenommen, da es die Funktionsnummer
 * des Syscalls enthalten muß.
 *
 * @param esp Interrupt-Stackframe des vom RPC-Handler zurückkehrenden Tasks
 */
void return_from_rpc(struct int_stack_frame ** esp)
{
    rpc_t* rpc = list_pop(current_task->rpcs);
    struct int_stack_frame* callee_isf = *esp;

    // Wenn der Task vom RPC-Handler zurückkehrt, obwohl der Handler
    // gar nicht aufgerufen wurde, läuft was schief
    if (rpc == NULL) {
        if(debug_test_flag(DEBUG_FLAG_STACK_BACKTRACE)) {
            stack_backtrace_ebp(callee_isf->ebp, callee_isf->eip);
        }
        abort_task("Unerwartete Rueckkehr vom RPC-Handler");
    }
    
    // Wiederherstellen der Register
    callee_isf->eip = rpc->old_eip; 
    callee_isf->esp = rpc->old_esp;
    callee_isf->eax = rpc->eax; 
    callee_isf->eflags = rpc->eflags;

    // Wechsel zum aufrufenden Task
    schedule_to_task(rpc->caller, (dword*) esp);

    free(rpc);
}
