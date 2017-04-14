/*
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
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

#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stdbool.h>
#include "types.h"
#include "syscallno.h"
#include "syscall_structs.h"
typedef struct 
{
    vaddr_t virt;
    paddr_t phys;
} dma_mem_ptr_t;

typedef struct
{
    uint32_t total;
    uint32_t free;
} memory_info_t;

//int puts(const char* s);
int syscall_putsn(unsigned int n, const char* s);


void* mem_allocate(uint32_t size, uint32_t flags);
void *mem_allocate_physical(uint32_t size, uint32_t position, uint32_t flags);
dma_mem_ptr_t mem_dma_allocate(uint32_t size, uint32_t flags);
bool mem_free(void* address, uint32_t size);
void mem_free_physical(void* address, uint32_t size);
memory_info_t memory_info(void);
void* get_phys_addr(void* address);

uint32_t create_shared_memory(uint32_t size);
void *open_shared_memory(uint32_t id);
void close_shared_memory(uint32_t id);
ssize_t shared_memory_size(uint32_t id);

bool request_ports(uint32_t port, uint32_t length);
bool release_ports(uint32_t port, uint32_t length);

void yield(void);
void wait_for_rpc(void);
void syscall_p(void);
void syscall_v(void);

#define p() do { __sync_synchronize(); syscall_p(); } while (0)
#define v() do { __sync_synchronize(); syscall_v(); } while (0)


void set_rpc_handler(void (*rpc_handler)(void));
void add_intr_handler(uint32_t intr);

void rpc(pid_t pid);
int send_message(pid_t pid, uint32_t function, uint32_t correlation_id,
    uint32_t len, char* data);
void v_and_wait_for_rpc(void);

pid_t get_pid(void);
pid_t get_parent_pid(pid_t pid);
pid_t create_process(uint32_t initial_eip, uid_t uid, const char* path,
    pid_t parent_pid);
void destroy_process(void);
void init_child_page (pid_t pid, void* dest, void* src, size_t size);
void init_child_page_copy (pid_t pid, void* dest, void* src, size_t size);
int init_child_ppb(pid_t pid, int shm);
void unblock_process(pid_t pid);
char* get_cmdline(void);

bool vm86_int(vm86_regs_t *regs, uint32_t *shm);
int bios_int(int intr, vm86_regs_t* regs, uint32_t* shm);

task_info_t* enumerate_tasks(void);

uint64_t get_tick_count(void);

void syscall_timer(uint32_t timer_id, uint32_t usec);

void syscall_debug_stacktrace(pid_t pid);

tid_t create_thread(uint32_t start, void *arg);
void exit_thread(void);
tid_t get_tid(void);

// LIO2

typedef lio_usp_stream_t lio_stream_t;
typedef lio_usp_resource_t lio_resource_t;
typedef lio_usp_tree_t lio_tree_t;

/* FIXME Duplikat von lio_usp_dir_entry in syscall_structs.h */
struct lio_dir_entry {
    lio_usp_resource_t resource;
    char               name[LIO_MAX_FNLEN + 1];
    struct lio_stat    stat;
} __attribute__((packed));

/**
 * Sucht eine Ressource anhand eines Pfads und gibt ihre ID zurück.
 *
 * @param follow_symlink Wenn follow_symlink true ist und der Pfad auf eine
 * symbolische Verknüpfung zeigt, dann wird die Verknüfung aufgelöst. Ist
 * follow_symlink false, wird in diesem Fall die Verknüpfung selbst
 * zurückgegeben.
 *
 * @return Ressourcen-ID bei Erfolg, negativ im Fehlerfall. Insbesondere:
 *
 *      -ENOENT     Es wurde keine Ressource mit dem gegebenen Pfad gefunden
 */
lio_resource_t lio_resource(const char* path, bool follow_symlink);

/**
 * Erstellt eine neue Pipe und gibt Streams auf beide Enden zurück.
 *
 * @param reader Wird bei Erfolg auf das erste Ende der Pipe gesetzt
 * @param writer Wird bei Erfolg auf das zweite Ende der Pipe gesetzt
 * @param bidirectional Wenn false, dann kann auf writer nur geschrieben und
 * auf reader nur gelesen werden. Wenn true, dann funktioniert auch die andere
 * Richtung.
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall
 */
int lio_pipe(lio_stream_t* reader, lio_stream_t* writer, bool bidirectional);

/**
 * Öffnet eine Ressource und gibt die ID des entstandenen Streams zurück.
 *
 * @param resource ID der zu öffnenden Ressource
 * @param flags Bitmaske aus LIO_*-Flags
 *
 * @return Stream-ID bei Erfolg, negativ im Fehlerfall. Insbesondere:
 *
 *      -EINVAL     Es gibt keine Ressource mit der gegebenen ID
 *      -EACCES     Die Ressource unterstützt ein gesetztes Flag nicht
 */
lio_stream_t lio_open(lio_resource_t resource, int flags);

/**
 * Setzt einen neuen Stream zusammen, der die Eingaberessource von read und
 * die Ausgaberessource von write benutzt.
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall
 */
lio_stream_t lio_composite_stream(lio_stream_t read, lio_stream_t write);

/**
 * Erstellt eine zusätzliche Stream-ID für einen Stream.
 *
 * @param source Der zu duplizierende Stream
 * @param dest Neue ID, die  der Stream erhalten soll, oder -1 für eine
 * beliebige freie ID. Wenn diese ID bereits einen Stream bezeichnet, wird
 * dieser geschlossen.
 *
 * @return Neue Stream-ID bei Erfolg, negativ im Fehlerfall. Insbesondere:
 *
 *      -EBADF      Ein der Stream-IDs ist ungültig
 */
lio_stream_t lio_dup(lio_stream_t source, lio_stream_t dest);

/**
 * Gibt Informationen zu einer Ressource zurück.
 *
 * @param resource ID der Ressource
 * @param sbuf Puffer für die Informationen
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall. Insbesondere:
 *
 *      -EINVAL     Es gibt keine Ressource mit der gegebenen ID
 */
int lio_stat(lio_resource_t resource, struct lio_stat* sbuf);

/**
 * Schließt einen Stream.
 *
 * @param s Der zu schließende Stream
 * @return 0 bei Erfolg, negativ im Fehlerfall.
 */
int lio_close(lio_stream_t s);

/**
 * Gibt einen Stream an einen anderen Prozess ab. Der Stream wird in diesem
 * Prozess dabei ungültig.
 *
 * @param s Der abzugebenden Stream
 * @param pid Zielprozess, der den Stream erhalten soll
 *
 * @return Stream-ID im Zielprozess bei Erfolg, negativ im Fehlerfall.
 * Insbesondere:
 *
 *      -EINVAL     Es gibt keinen Prozess mit der gegebenen PID
 *      -EBADF      Die Stream-ID ist ungültig
 */
lio_stream_t lio_pass_fd(lio_stream_t s, pid_t pid);

/**
 * Gibt den Prozess zurück, der den Stream an den aktuellen Prozess übergeben
 * hat.
 *
 * @param s Der zu prüfende Stream
 * @return Falls der Stream von einem anderen Prozess übergeben wurde, dessen
 * PID. Falls er von diesem Prozess geöffnet wurde, die eigene PID. Negativ
 * im Fehlerfall, insbesondere:
 *
 *      -EBADF      Die Stream-ID ist ungültig
 */
int lio_stream_origin(lio_stream_t s);

/**
 * Liest aus dem Stream.
 *
 * @param s Auszulesender Stream
 * @param offset Position, ab der gelesen werden soll, in Bytes vom
 * Dateianfang. Wenn in @flags LIO_REQ_FILEPOS gesetzt ist, wird die Angabe
 * ignoriert und stattdessen von der aktuellen Position des Dateizeigers
 * gelesen.
 * @param bytes Zu lesende Bytes
 * @param buf Zielpuffer, in dem die ausgelesenen Daten abgelegt werden
 * @param flags Bitmaske von lio_req_flags
 *
 * @return Die Anzahl der erfolgreich gelesenen Bytes ("short reads" sind
 * möglich) oder negativ, falls sofort ein Fehler aufgetreten ist.
 */
ssize_t lio_readf(lio_stream_t s, uint64_t offset, size_t bytes,
    void* buf, int flags);

/**
 * Liest aus dem Stream ab der aktuellen Position des Dateizeigers und bewegt
 * den Dateizeiger hinter das letzte gelesene Byte.
 *
 * @param s Auszulesender Stream
 * @param bytes Zu lesende Bytes
 * @param buf Zielpuffer, in dem die ausgelesenen Daten abgelegt werden
 *
 * @return Die Anzahl der erfolgreich gelesenen Bytes ("short reads" sind
 * möglich) oder negativ, falls sofort ein Fehler aufgetreten ist.
 */
ssize_t lio_read(lio_stream_t s, size_t bytes, void* buf);

/**
 * Liest aus dem Stream ab der gegebenen Position.
 *
 * @param s Auszulesender Stream
 * @param offset Position, ab der gelesen werden soll, in Bytes vom Dateianfang
 * @param bytes Zu lesende Bytes
 * @param buf Zielpuffer, in dem die ausgelesenen Daten abgelegt werden
 *
 * @return Die Anzahl der erfolgreich gelesenen Bytes ("short reads" sind
 * möglich) oder negativ, falls sofort ein Fehler aufgetreten ist.
 */
ssize_t lio_pread(lio_stream_t s, uint64_t offset, size_t bytes, void* buf);

/**
 * Schreibt in den Stream ab der aktuellen Position des Dateizeigers und bewegt
 * den Dateizeiger hinter das letzte geschriebene Byte.
 *
 * @param s Zu schreibender Stream
 * @param bytes Zu schreibende Bytes
 * @param buf Quellpuffer, dessen Inhalt geschrieben werden soll
 *
 * @return Die Anzahl der erfolgreich gegeschriebenen Bytes ("short writes"
 * sind möglich) oder negativ, falls sofort ein Fehler aufgetreten ist.
 */
ssize_t lio_write(lio_stream_t s, size_t bytes, const void* buf);

/**
 * Schreibt in den Stream ab der gegebenen Position.
 *
 * @param s Zu schreibender Stream
 * @param offset Position, ab der geschrieben werden soll, in Bytes vom
 * Dateianfang
 * @param bytes Zu schreibende Bytes
 * @param buf Quellpuffer, dessen Inhalt geschrieben werden soll
 *
 * @return Die Anzahl der erfolgreich gegeschriebenen Bytes ("short writes"
 * sind möglich) oder negativ, falls sofort ein Fehler aufgetreten ist.
 */
ssize_t lio_pwrite(lio_stream_t s, uint64_t offset, size_t bytes,
    const void* buf);

/**
 * Bewegt den Dateizeiger des Streams und gibt seine aktuelle Position zurück
 *
 * @param s Stream, dessen Dateizeiger verändert werden soll
 * @param offset Neue Dateizeigerposition als Offset in Bytes
 * @param whence Gibt an, wozu das Offset relativ ist (LIO_SEEK_*-Konstanten)
 *
 * @return Die neue Position des Dateizeigers in Bytes vom Dateianfang, oder
 * negativ im Fehlerfall.
 */
int64_t lio_seek(lio_stream_t s, uint64_t offset, int whence);

/**
 * Ändert die Dateigröße der Ressource eines Streams
 * TODO Wieso nimmt das einen Stream und keine Ressource?
 *
 * @param s Zu ändernder Stream
 * @param size Neue Dateigröße in Bytes
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall.
 */
int lio_truncate(lio_stream_t s, uint64_t size);

/**
 * Führt einen ressourcenspezifischen Befehl auf einem Stream aus.
 *
 * @param s Stream, auf den sich der Befehl bezieht
 * @param cmd Auszuführender Befehl (LIO_IOCTL_*-Konstanten)
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall.
 */
int lio_ioctl(lio_stream_t s, int cmd);

/**
 * Liest Einträge einer Verzeichnisressource aus.
 *
 * @param res Auszulesende Verzeichnisressource
 * @param start Index des ersten zurückzugebenden Verzeichniseintrags
 * @param num Maximale Anzahl zurückzugebender Verzeichniseinträge
 * (Puffergröße)
 * @param buf Array von Verzeichniseinträgen, die befüllt werden
 *
 * @return Anzahl der zurückgegebenen Verzeichniseinträge, oder negativ im
 * Fehlerfall.
 */
ssize_t lio_read_dir(lio_resource_t res, size_t start, size_t num,
    struct lio_dir_entry* buf);

/**
 * Erstellt eine neue Datei.
 *
 * @param parent Verzeichnis, das die neue Datei enthalten soll
 * @param name Name der neuen Datei
 *
 * @return Ressourcen-ID der neuen Datei bei Erfolg, negativ im Fehlerfall.
 */
lio_resource_t lio_mkfile(lio_resource_t parent, const char* name);

/**
 * Erstellt ein neues Verzeichnis.
 *
 * @param parent Verzeichnis, das das neue Unterverzeichnis enthalten soll
 * @param name Name des neuen Verzeichnisses
 *
 * @return Ressourcen-ID des neuen Verzeichnisses bei Erfolg, negativ im
 * Fehlerfall.
 */
lio_resource_t lio_mkdir(lio_resource_t parent, const char* name);

/**
 * Erstellt eine neue symbolische Verknüpfung
 *
 * @param parent Verzeichnis, das die neue Verknüofung enthalten soll
 * @param name Name der neuen Verknüpfung
 *
 * @return Ressourcen-ID der neuen Verknüpfung bei Erfolg, negativ im
 * Fehlerfall.
 */
lio_resource_t lio_mksymlink(lio_resource_t parent,
    const char* name, const char* target);

/*
 * Sofern die Ressource grundsätzlich auf persistentem Speicher liegt, aber
 * unter Umständen bisher nur in einem nichtpersistentem Cache liegt, werden
 * alle Änderungen, die an diesem Stream durchgeführt worden sind, persistent
 * gemacht.
 *
 * @param s Zurückzuschreibender Stream
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall.
 */
int lio_sync(lio_stream_t s);

/*
 * Löscht den Verzeichniseintrag einer Ressource aus ihrem Verzeichnis. Wenn
 * es keine weiteren Verweise (Hardlinks) auf die Ressource mehr gibt, wird sie
 * gelöscht.
 *
 * @param parent Verzeichnis, das die zu löschende Ressource enthält
 * @param name Dateiname der zu löschenden Ressource
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall.
 */
int lio_unlink(lio_resource_t parent, const char* name);

/**
 * Schreibt alle Änderungen im Kernelcache zurück.
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall.
 */
int lio_sync_all(void);

/**
 * Versucht einen Service zu finden, der die angegebene Ressource als
 * Pipequelle verwenden kann (z.B. passendes Dateisystem für Image)
 *
 * @return 0 bei Erfolg, negativ im Fehlerfall
 */
int lio_probe_service(lio_resource_t res,
                      struct lio_probe_service_result* ret);

// LIO2-Server

/**
 * Neuen LIO-Service registrieren.
 *
 * @param name   Service-Name
 * @param buffer Zeiger auf den zu benutzenden Ringpuffer fuer die Operationen.
 *               Dieser sollte mit sinnvollen Werten initialisiert sein (z.B
 *               memset(buffer, 0, buffer_size)
 * @param size   Groesse des Puffers
 *
 * @return 0 bei Erfolg, < 0  im Fehlerfall
 */
int lio_srv_service_register(const char* name, void* buffer, size_t size);

/**
 * LIO-Befehlsring für einen gegebenen Tree einrichten
 *
 * @param tree   ID des Trees
 * @param tid    TID des Threads, der den Ring abarbeitet
 * @param buffer Zeiger auf den zu benutzenden Ringpuffer fuer die Operationen.
 *               Dieser sollte mit sinnvollen Werten initialisiert sein (z.B
 *               memset(buffer, 0, buffer_size)
 * @param size   Groesse des Puffers
 *
 * @return 0 bei Erfolg, < 0  im Fehlerfall
 */
int lio_srv_tree_set_ring(lio_usp_tree_t tree, tid_t tid,
                          void* buffer, size_t size);

/**
 * Ressource fuer Kernel bereitstellen, dabei wird der id-Member eingefuellt.
 *
 * @return 0 bei Erfolg, < 0 im Fehlerfall
 */
int lio_srv_res_upload(struct lio_server_resource* resource);

/**
 * Neuen Knoten erstellen.
 *
 * @param parent   Elternressource in der der Knoten angelegt werden soll
 * @param resource Ressource auf die der Knoten verweisen soll
 * @param name     Namen des neuen Knoten
 */
int lio_srv_node_add(lio_usp_resource_t parent, lio_usp_resource_t resource,
    const char* name);

/**
 * Knoten loeschen.
 *
 * @param parent Elternressource in der der Kindknoten geloescht werden soll
 * @param name   Name des zu loeschenden Knotens
 *
 * @return 0 bei Erfolg, < 0 im Fehlerfall
 */
int lio_srv_node_remove(lio_usp_resource_t parent, const char* name);

/**
 * Kernel ueber abgearbeitete Operation informieren. Das USP-Bit in der
 * Operation muss geloescht sein.
 *
 * @param op Fertig abgearbeitete Operation
 * @param status 0 im Erfolg, -errno im Fehlerfall
 * @param result Operationsabhaengig ein Rueckgabewert (z.B. Ressourcen-ID)
 */
void lio_srv_op_done(struct lio_op* op, int status, uint64_t result);

/**
 * Auf LIO-Operationen fuer diesen Prozess warten. Solange noch unbearbeitete
 * Operationen existieren, kehrt der Aufruf sofort zurueck.
 */
void lio_srv_wait(void);

#endif
