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
#include <errno.h>

#include "syscall.h"
#include "syscall_structs.h"
#include "mm.h"
#include "tasks.h"
#include "cpu.h"

/**
 * Neuen Prozess mit einem Thread erstellen. Der neue Task wird blockiert, bis
 * er vom Ersteller entblockt wird.
 *
 * @param start Start-Adresse
 * @param uid User-Id
 * @param cmdline Kommandozeile mit der der Prozess gestartet wurde
 * @param parent_pid PID des Elternprozesses; bei 0 wird die PID des
 * aufrufenden Prozesses selbst benutzt
 *
 * @return PID des neuen Tasks
 */
pid_t syscall_pm_create_process(vaddr_t start, uid_t uid, const char* cmdline,
    pid_t parent_pid)
{
    // Eltern-Prozess holen
    pm_process_t* parent;

    if (parent_pid == 0) {
        parent = current_process;
    } else {
        parent = pm_get(parent_pid);
    }

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
    pm_process_t* old_task = current_process;

    // Ein paar erste Datenstrukturen freigeben
    pm_prepare_destroy(old_task);

    // Task abgeben
    pm_scheduler_push(current_thread);

    // Den Prozess blockieren
    while (pm_block(old_task) == false);


    // Genau jetzt muss der Prozess geloescht werden: Der Thread ist nicht mehr
    // aktiv, und er darf auch nicht wieder aktiviert werden.
    pm_destroy(old_task);
}

/**
 * PID des aktuellen Prozesses ausfindig machen
 *
 * @return PID
 */
pid_t syscall_pm_get_pid()
{
    return current_process->pid;
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
        process = current_process;
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
    const char* cmdline = current_process->cmdline;
    if (cmdline == NULL) {
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
    current_process->memory_used -= num_pages << PAGE_SHIFT;
    process->memory_used += num_pages << PAGE_SHIFT;

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

        if (dest_phys != (paddr_t) NULL) {
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
 * Initialisiert den Prozessparameterblock eines Kindprozesses. Dazu wird der
 * gegebene Shared Memory in den Kindprozess gemappt und dem Kindprozess in den
 * Registern eax und edx ein Pointer auf den gemappten Bereich bzw. die Laenge
 * des gemappten Bereichs in Bytes angegeben.
 *
 * Dieser Syscall darf nur benutzt werden, solange der Prozess noch nicht
 * gestartet wurde.
 */
int syscall_init_ppb(pid_t pid, int shm_id)
{
    pm_process_t* process;
    void* ptr;
    size_t size;

    process = pm_get(pid);
    if (process == NULL) {
        return -ESRCH;
    }

    // FIXME Prüfen, dass der Prozess noch nicht laeuft und dass er von
    // current_process blockiert wird

    /* SHM in den Kindprozess mappen */
    ptr = shm_attach(process, shm_id);
    if (ptr == NULL) {
        return -EINVAL;
    }

    size = shm_size(shm_id);

    /* Register eintragen */
    return arch_init_ppb(process, shm_id, ptr, size);
}

/**
 * Die Kontrolle an einen anderen Task abgeben
 *
 * FIXME Diesen Code gibt es in genau dieser Form auch in im.c
 */
void syscall_pm_sleep(void)
{
    pm_thread_t* thread = current_thread;

    // Den aktuellen Thread an den Scheduler zurueckgeben
    pm_scheduler_push(thread);

    // Einen neuen Thread holen.
    current_thread = pm_scheduler_pop();
}

/**
 * Privilegierte Version von syscall_pm_sleep, die dem alten Task einen
 * spezifischen neuen Zustand gibt und zu einem bestimmten anderen Task
 * wechseln kann.
 */
void kern_syscall_pm_sleep(tid_t yield_to, int status)
{
    pm_thread_t* thread = NULL;

    current_thread->status = status;

    if (yield_to) {
        thread = pm_thread_get(NULL, yield_to);
    }

    if (thread && pm_thread_runnable(thread)) {
        pm_scheduler_get(thread);
        thread->status = PM_STATUS_RUNNING;
        current_thread = thread;
    } else {
        current_thread = pm_scheduler_pop();
    }
}

/**
 * Die Kontrolle an einen anderen Task abgeben und erst wieder aufwachen,
 * wenn ein RPC zu bearbeiten ist
 */
void syscall_pm_wait_for_rpc(void)
{
    pm_thread_t* thread = current_thread;
    thread->status = PM_STATUS_WAIT_FOR_RPC;
    current_thread = pm_scheduler_pop();
}

/**
 * Alle Prozesse auflisten
 *
 * @return Gibt einen Pointer auf neu allozierte Seiten zurueck, die
 * Informationen ueber alle laufenden Tasks enthalten. Der Pointer zeigt dabei
 * auf Daten vom Typ task_info_t.
 */
void* syscall_pm_enumerate_tasks(void)
{
    // Erst werden die Tasks gezaehlt, und die Groesse der
    // Informationen mit den Kommandozeilen wird errechnet.
    size_t task_count = 0;
    size_t result_size = sizeof(task_info_t);
    pm_process_t* task;
    unsigned int i;

    for (i = 0; (task = list_get_element_at(process_list, i)); i++) {
        task_count++;
        result_size += sizeof(task_info_task_t) + strlen(task->cmdline) + 1;
    }

    // Anzahl der Seiten berechnen, die die Informationen benoetigen.
    size_t result_page_count = PAGE_ALIGN_ROUND_UP(result_size) / PAGE_SIZE;

    // Entsprechend auch die Speichernutzung aktualisieren
    current_process->memory_used += result_page_count << PAGE_SHIFT;
    
    // Jetzt wird eine freie Stelle im Adressraum des Prozesses
    // gesucht, wo die Task-Infos hingemappt werden koennen
    task_info_t* task_info = mmc_valloc(&mmc_current_context(),
        result_page_count, MM_FLAGS_USER_DATA);

    // Der Groessen-Eintrag ist nur da, damit der Task die Pages
    // freigeben koennte.
    task_info->info_size = result_size;
    task_info->task_count = task_count;

    // Dieser Pointer zeigt direkt hinter das Array mit den
    // Task-Informationen. Dort werden die Kommandozeilen
    // hintereinander gespeichert, und aus den Task-Strukturen wird auf
    // sie verwiesen.
    char* cmdlines = (char*) task_info->tasks;
    cmdlines += task_count * sizeof(task_info_task_t);

    // Jetzt werden die Infos eingefuellt.
    for (i = 0; (task = list_get_element_at(process_list, i)); i++) {
        task_info->tasks[i].pid = task->pid;
        task_info->tasks[i].status = task->status;

        // TODO Ergibt strenggenommen keinen Sinn, evtl. ersten Thread?
        task_info->tasks[i].eip = 0;

        // Wenn der Task keinen Eltern-Task hat muessen wir aufpassen,
        // damit wir keinen Pagefault produzieren.
        if (task->parent == NULL) {
            task_info->tasks[i].parent_pid = 0;
        } else {
            task_info->tasks[i].parent_pid = task->parent->pid;
        }

        // Die Kommandozeile inklusive Nullbyte kopieren
        size_t cmdline_size = strlen(task->cmdline) + 1;
        strncpy(cmdlines, task->cmdline, cmdline_size);

        // Den Pointer fuer die Kommandozeile setzen
        task_info->tasks[i].cmdline = cmdlines;

        // Den Zielpointer fuer die naechste Kommandozeile direkt
        // hinter die aktuelle setzen.
        cmdlines += cmdline_size;

        task_info->tasks[i].memory_used = task->memory_used;
    }

    return task_info;
}

#if CONFIG_ARCH == ARCH_I386
/**
 * TID des aktuellen Threads ausfindig machen
 *
 * @return TID des aktuellen Threads
 */
tid_t syscall_pm_get_tid()
{
    int i = 0;

    pm_thread_t *thread;

    while ((thread = list_get_element_at(current_process->threads, i++))) {
        if (thread->status == PM_STATUS_RUNNING) {
            return thread->tid;
        }
    }

    return -1;
}

/**
 * Neuen Thread in einem Prozess erstellen. Der neue Task wird blockiert, bis
 * er vom Ersteller entblockt wird.
 *
 * @param start Start-Adresse
 * @param arg Argument für den Thread
 *
 * @return TID des neuen Tasks
 */
tid_t syscall_pm_create_thread(vaddr_t start, void *arg)
{
    pm_thread_t *thread;

    thread = pm_thread_create(current_process, start);
    if (thread == NULL) {
        return -1;
    }

    while (pm_thread_block(thread) == false);

    interrupt_stack_frame_t *isf = (interrupt_stack_frame_t *)thread->user_isf;

    /* Parameter auf den Userspace-Stack kopieren */
    isf->esp -= sizeof(void *);
    *((void **)(isf->esp)) = arg;

    /* Rücksprungadresse leer lassen */
    isf->esp -= sizeof(void *);

    while (pm_thread_unblock(thread) == true);

    return thread->tid;
}

void syscall_pm_exit_thread(void)
{
    pm_thread_t *thread;
    tid_t tid;
    int i = 0;

    tid = syscall_pm_get_tid();

    if (tid == -1) {
        // TODO Kann das wirklich passieren?
        return;
    }

    while ((thread = list_get_element_at(current_process->threads, i++))) {
        if (thread->tid == tid) {
            pm_scheduler_push(thread);
            pm_thread_block(thread);
            pm_thread_destroy(thread);
            break;
        }
    }
}
#endif
