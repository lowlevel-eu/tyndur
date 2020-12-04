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

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <types.h>
#include <stdint.h>
#include <syscall_structs.h>

#include "cpu.h"

// Ist das wirklich die korrekte Definition? Dieser Typ sollte der Groesse der
// Werte die auf dem Stack gespeichert werden entsprechen.
// FIXME: Doppelte Definition! (modules/include/syscall.h)
typedef uintptr_t syscall_arg_t;

typedef struct {
    /// Pointer auf den Handler des Syscalls oder NULL, falls dieser Syscall
    /// nicht existiert.
    void* handler;
    
    /// Anzahl der Argumente die der Syscall benoetigt
    size_t arg_count;

    /// true, wenn der Syscall nur aus dem Kernelspace aufgerufen werden darf
    bool privileged;
} syscall_t;

/// In diesem Array werden alle Syscalls gespeichert.
extern syscall_t syscalls[];


/// Architektur-Abhaengiger Handler, der die eigentlichen Handler aufruft
void syscall_arch(machine_state_t* isf);


/// Einen neuen Syscall verfuegbar machen
void syscall_register(syscall_arg_t number, void* handler, size_t arg_count);

/// Syscalls initialisieren
void syscall_init(void);

// ###########################################################################
// SYSCALLS
// ###########################################################################

// Speicherverwaltung
/// Einen Speicherblock allokieren.
vaddr_t syscall_mem_allocate(size_t bytes, syscall_arg_t flags, paddr_t* phys);

/// Physischen Speicher mappen
vaddr_t syscall_mem_allocate_physical(
    size_t bytes, paddr_t position, syscall_arg_t flags);

/// Einen Speicherblock freigeben
void syscall_mem_free(vaddr_t start, size_t bytes);

/// Physischen Speicher freigeben
void syscall_mem_free_physical(vaddr_t start, size_t bytes);

/// Freie Pages und Gesamtspeicher zurueckgeben
void syscall_mem_info(uint32_t* sum_pages, uint32_t* free_pages);


// Prozessverwaltung
/// PID des aktuellen Prozesses abfragen
pid_t syscall_pm_get_pid(void);

/// PID des Elternprozesses ausfindig machen
pid_t syscall_pm_get_parent_pid(pid_t pid);

/// Befehlszeile des aktuellen Prozesses abfragen
const char* syscall_pm_get_cmdline(void);

/// Kritischen Abschnitt betreten
int syscall_pm_p(void);

/// Kritischen Abschnitt verlassen
int syscall_pm_v(pid_t pid);

/// Kritischen Abschnitt verlassen und auf RPC warten
int syscall_pm_v_and_wait_for_rpc(void);

/// Die Kontrolle an einen anderen Task abgeben
void syscall_pm_sleep(void);

/// Die Kontrolle an einen anderen Task abgeben
void kern_syscall_pm_sleep(tid_t yield_to, int status);

/// Warten, bis ein RPC zu bearbeiten ist
void syscall_pm_wait_for_rpc(void);

/// Prozess erstellen
pid_t syscall_pm_create_process(vaddr_t start, uid_t uid,
    const char* cmdline, pid_t parent_pid);

/// Aktuellen Prozess beenden
void syscall_pm_exit_process(void);

/// Alle Prozesse auflisten
void* syscall_pm_enumerate_tasks(void);


/// Thread erstellen
tid_t syscall_pm_create_thread(vaddr_t start, void *arg);

/// ID des aktuellen Threads abfragen
tid_t syscall_pm_get_tid(void);

/// Thread beenden
void syscall_pm_exit_thread(void);

/// Mutex nehmen
void syscall_mutex_lock(int* mutex);

/// Mutex freigeben
void syscall_mutex_unlock(int* mutex);


/// Speicher an einen anderen Prozess uebergeben
void syscall_init_child_page(pid_t pid, vaddr_t src, vaddr_t dest,
    size_t size);

/// Initialisiert den Prozessparameterblock eines Kindprozesses
int syscall_init_ppb(pid_t pid, int shm_id);
int arch_init_ppb(pm_process_t* process, int shm_id, void* ptr, size_t size);

/// IO-Ports anfordern
int syscall_io_request_port(uint32_t port, uint32_t length);

/// IO-Ports freigeben
int syscall_io_release_port(uint32_t port, uint32_t length);

/// Aktuelle Zeit abfragen (Mikrosekunden seit Systemstart)
uint64_t syscall_get_tick_count(void);


// SHM
/// Neuen Shared Memory reservieren
uint32_t syscall_shm_create(size_t size);

/// Bestehenden Shared Memory oeffnen
void* syscall_shm_attach(uint32_t id);

/// Shared Memory schliessen
void syscall_shm_detach(uint32_t id);

/// Größe eines Shared Memory zurückgeben
int32_t syscall_shm_size(uint32_t id);

/// Einen Timer anlegen
void syscall_add_timer(uint32_t timer_id, uint32_t usec);

// RPC
/// RPC-Handler registrieren
void syscall_set_rpc_handler(vaddr_t address);

/// RPC durchfuehren
int syscall_fastrpc(pid_t callee_pid, size_t metadata_size, void* metadata,
    size_t data_size, void* data);

/// Von einem RPC zurueckkehren
void syscall_fastrpc_ret(void);

/// Interrupt registrieren
void syscall_add_interrupt_handler(uint32_t intr);


// LIO
/// Ressource suchen
void syscall_lio_resource(const char* path, size_t path_len, int flags,
    lio_usp_resource_t* res_id);

/// Informationen über eine Ressource abfragen
int syscall_lio_stat(lio_usp_resource_t* resid, struct lio_stat* sbuf);

/// Ressource öffnen
void syscall_lio_open(lio_usp_resource_t* resid, int flags,
    lio_usp_stream_t* stream_id);

/// Pipe erstellen
int syscall_lio_pipe(lio_usp_stream_t* stream_reader,
                     lio_usp_stream_t* stream_writer,
                     bool bidirectional);

/// Ein- und Ausgabestream zusammensetzen
int syscall_lio_composite_stream(lio_usp_stream_t* read,
                                 lio_usp_stream_t* write,
                                 lio_usp_stream_t* result);

/// Stream schließen
int syscall_lio_close(lio_usp_stream_t* stream_id);

/// Zusätzliche ID für Stream erstellen
int syscall_lio_dup(lio_usp_stream_t* source, lio_usp_stream_t* dest);

/// Stream an anderen Prozess weitergeben
int syscall_lio_pass_fd(lio_usp_stream_t* stream_id, pid_t pid);

/// PID des Prozesses, der den Stream dem aktuellen übergeben hat, auslesen
int syscall_lio_stream_origin(lio_usp_stream_t* stream_id);

/// Aus Stream lesen
void syscall_lio_read(lio_usp_stream_t* stream_id, uint64_t* offset,
    size_t bytes, void* buffer, int updatepos, ssize_t* result);

/// In Stream schreiben
void syscall_lio_write(lio_usp_stream_t* stream_id, uint64_t* offset,
    size_t bytes, const void* buffer, int updatepos, ssize_t* result);

/// Cursorposition in der Datei ändern
void syscall_lio_seek(lio_usp_stream_t* stream_id, int64_t* offset,
    int whence, int64_t* result);

/// Dateigröße ändern
int syscall_lio_truncate(lio_usp_stream_t* stream_id, uint64_t* size);

/// Veränderte Blocks schreiben
int syscall_lio_sync(lio_usp_stream_t* stream_id);

/// Verzeichnisinhalt auslesen
void syscall_lio_read_dir(lio_usp_resource_t* res, size_t start, size_t num,
    struct lio_usp_dir_entry* dent, ssize_t* result);

/// Ressourcenspezifischen Befehl ausführen
int syscall_lio_ioctl(lio_usp_stream_t* stream_id, int cmd);

/// Neue Datei anlegen
void syscall_lio_mkfile(lio_usp_resource_t* parent, const char* name,
    size_t name_len, lio_usp_resource_t* result);

/// Neues Verzeichnis anlegen
void syscall_lio_mkdir(lio_usp_resource_t* parent, const char* name,
    size_t name_len, lio_usp_resource_t* result);

/// Neuen Symlink anlegen
void syscall_lio_mksymlink(lio_usp_resource_t* parent, const char* name,
    size_t name_len, const char* target, size_t target_len,
    lio_usp_resource_t* result);

/// Verzeichniseintrag löschen
int syscall_lio_unlink(lio_usp_resource_t* parent, const char* name,
    size_t name_len);

/// Alle Blocks rausschreiben, die sich im Cache befinden
int syscall_lio_sync_all(int soft);

/// Service finden, der die Ressource als Pipequelle akzeptiert
int syscall_lio_probe_service(lio_usp_resource_t* res,
                              struct lio_probe_service_result* probe_data);

// LIO-Server
/// Neuen lio-Service registrieren
int syscall_lio_srv_service_register(const char* name, size_t name_len,
    void* buffer, size_t size);

/// Neuen Tree-Ring registrieren
int syscall_lio_srv_tree_set_ring(lio_usp_tree_t* tree_id, tid_t tid,
    void* buffer, size_t size);

/// Ressource fuer Kernel bereitstellen
int syscall_lio_srv_res_upload(struct lio_server_resource* resource);

/// Neuen Knoten erstellen
int syscall_lio_srv_node_add(lio_usp_resource_t* parent,
    lio_usp_resource_t* resource, const char* name, size_t name_len);

/// Knoten loeschen
int syscall_lio_srv_node_remove(lio_usp_resource_t* parent, const char* name,
    size_t name_len);

/// Kernel ueber abgearbeitete Operation informieren
void syscall_lio_srv_op_done(struct lio_op* op, int status, uint64_t* result);

/// Auf LIO-Operationen fuer diesen Prozess warten
void syscall_lio_srv_wait(void);

/// Cache-Blocks mit Uebersetzungstabelle anfordern
int syscall_lio_srv_request_translated_blocks(lio_usp_resource_t* res,
    uint64_t* offset, size_t num, uint64_t* table);


// Diverse
/// Textausgabe ueber den Kernel
void syscall_putsn(int char_count, char* source);

/// -ENOSYS
int syscall_vm86_old(void* regs, uint32_t* memory);

/// BIOS-Interrupt ausfuehren
int syscall_vm86(uint8_t intr, void* regs, uint32_t* memory);

#endif //ifndef _SYSCALL_H_

