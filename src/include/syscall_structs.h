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

#ifndef _SYSCALL_STRUCTS_H_
#define _SYSCALL_STRUCTS_H_

#include <stdint.h>
#include <types.h>

typedef struct
{
    size_t count;
    struct {
        void*   start;
        size_t  size;
        char*   cmdline;
    } modules[];
} init_module_list_t;

typedef struct
{
    pid_t pid;
    pid_t parent_pid;

    uint32_t status;
    uint32_t eip;
    uint32_t memory_used;

    const char* cmdline;
} task_info_task_t;

typedef struct
{
    size_t task_count;
    size_t info_size;

    task_info_task_t tasks[];
} task_info_t;

typedef struct
{
    uint16_t ax;
    uint16_t bx;
    uint16_t cx;
    uint16_t dx;
    uint16_t si;
    uint16_t di;
    uint16_t ds;
    uint16_t es;
} vm86_regs_t;

// LIO2
#define LIO_OFFSET_SPARSE (-1ULL)

/** ID eines LostIO-Streams */
typedef int64_t lio_usp_stream_t;

/** ID einer LostIO-Ressource */
typedef int64_t lio_usp_resource_t;

/** ID eines LostIO-Verzeichnisbaums */
typedef int64_t lio_usp_tree_t;


struct lio_probe_service_result {
    char service[32];
    char volname[32];
} __attribute__((packed));

/**
 * Beschreibt als Bitmaske den Modus, in dem ein Stream arbeiten soll.
 */
enum lio_flags {
    /**
     * Die Ressource soll beim Öffnen auf Größe 0 gekürzt werden. Nur
     * gültig, wenn die Datei auch zum Schreiben geöffnet wird.
     */
    LIO_TRUNC       =   1,

    /** Die Ressource soll zum Lesen geöffnet werden */
    LIO_READ        =   2,

    /** Die Ressource soll zum Schreiben geöffnet werden */
    LIO_WRITE       =   4,

    /**
     * Die Ressource soll im Writethrough-Modus geöffnet werden, d.h. alle
     * Änderungen werden sofort persistent gemacht.
     */
    /* TODO LIO_WRTHROUGH   =  32, */

    /** Der Cache soll nicht benutzt werden */
    /* TODO LIO_NOCACHE     =  64, */

    /**
     * Wenn auf die Ressource geschrieben oder von ihr gelesen werden soll, sie
     * aber nicht bereit ist, soll die Anfrage blockieren anstatt einen Fehler
     * zurückzugeben.
     *
     * TODO Short Reads/Writes möglich?
     */
    /* TODO LIO_BLOCKING    = 128, */

    /**
     * Der Inhalt der Ressource ist nicht reproduzierbar (z.B. Netzwerk).
     * Er muss im Cache behalten werden, solange er benutzt wird.
     */
    /* TODO LIO_VOLATILE    = 256, */

    /**
     * Muss zum Öffnen von Symlinks gesetzt sein, darf für andere Ressourcen
     * nicht gesetzt sein.
     */
    LIO_SYMLINK     = 512,
};

/**
 * Gibt Optionen zu einem einzelnen Request an.
 */
enum lio_req_flags {
    /** Der Dateizeiger wird nach der Operation bewegt, Offset ignoriert */
    LIO_REQ_FILEPOS         = 1,

    /** Die Operation blockiert, wenn sie sonst nichts tun würde */
    LIO_REQ_BLOCKING        = 2,
};


/**
 * Gibt an, realtiv zu welcher Position in der Datei das lio_seek-Offset
 * zu interpretieren ist.
 */
enum lio_seek_whence {
    /** Offset ist relativ zum Dateianfang */
    LIO_SEEK_SET = 0,

    /** Offset ist relativ zur aktuellen Position des Dateizeigers */
    LIO_SEEK_CUR = 1,

    /** Offset ist relativ zum Dateiende */
    LIO_SEEK_END = 2,
};

/**
 * Beschreibt als Bitmaske Fähgikeiten einer Ressource in
 * struct lio_stat.flags
 */
enum lio_stat_flags {
    /** Die Ressource ist lesbar */
    LIO_FLAG_READABLE   = 1,

    /** Die Ressource ist schreibbar */
    LIO_FLAG_WRITABLE  = 2,

    /** Die Ressource kann aufgelistet werden (ist ein Verzeichnis) */
    LIO_FLAG_BROWSABLE  = 4,

    /** Die Ressource verweist auf eine andere Ressource (Symlink) */
    LIO_FLAG_RESOLVABLE = 8,

    /** Das Verzeichnis ist schreibbar (Kindressourcen anlegen/löschen) */
    LIO_FLAG_CHANGEABLE = 16,

    /** Es kann geändert werden, auf welche Ressource verwiesen wird */
    LIO_FLAG_RETARGETABLE = 32,

    /** Dateizeiger von Streams auf die Ressource können geändert werden */
    LIO_FLAG_SEEKABLE = 64,

    /** Die Ressource ist eine (bidirektionale) Pipe */
    LIO_FLAG_PIPE = 128,
};

enum lio_ioctl_cmds {
    /** Gibt Erfolg zurück, wenn die Ressource ein Terminal ist */
    LIO_IOCTL_ISATTY = 1,
};

/**
 * Beschreibt eine Ressource
 */
struct lio_stat {
    /** Größe der Ressource in Bytes */
    uint64_t size;

    /** Bitmaske aus enum lio_stat_flags */
    uint32_t flags;
} __attribute__((packed));

/** Maximale Länge eines Dateinamens */
#define LIO_MAX_FNLEN 255

/** Beschreibt einen Verzeichniseintrag */
struct lio_usp_dir_entry {
    /** ID der Ressource (zur Benutzung mit lio_open) */
    lio_usp_resource_t resource;

    /** Dateiname der Ressource */
    char               name[LIO_MAX_FNLEN + 1];

    /** lio_stat-kompatible Beschreibung der Ressource */
    struct lio_stat    stat;
} __attribute__((packed));

/// Opcodes fuer Server-Operationen
enum lio_opcode {
    /**
     * Diese Operation ist ungueltig (und somit existieren auch keine weiteren)
     */
    LIO_OP_INVALID = 0,

    /**
     * NOP, kann fuer Padding im Ringpuffer benutzt werden
     */
    LIO_OP_NOP = 1,

    /**
     * Root-Ressource eines Dateisystems laden
     */
    LIO_OP_LOAD_ROOT = 2,

    /**
     * Kind-Ressourcen einer Ressource laden
     */
    LIO_OP_LOAD_CHILDREN = 3,

    /**
     * Datenbloecke von Ressource lesen
     */
    LIO_OP_READ = 4,

    /**
     * Datenbloecke in Ressource schreiben
     */
    LIO_OP_WRITE = 5,

    /**
     * Neue Datei erstellen
     */
    LIO_OP_MAKE_FILE = 6,

    /**
     * Neues Verzeichnis erstellen
     */
    LIO_OP_MAKE_DIR = 7,

    /**
     * Dateigroesse setzen
     */
    LIO_OP_TRUNCATE = 8,

    /**
     * Verzeichniseintrag loeschen
     */
    LIO_OP_UNLINK = 9,

    /**
     * Pipe erstellen
     */
    LIO_OP_PIPE = 10,

    /**
     * Symlink erstellen
     */
    LIO_OP_MAKE_SYMLINK = 11,

    /**
     * Blockbereich allozieren
     */
    LIO_OP_ALLOCATE = 12,

    /**
     * Ggf. vorhandene Caches zurückschreiben
     */
    LIO_OP_SYNC = 13,

    /**
     * Uebersetzungstabelle befuellen
     */
    LIO_OP_MAP = 14,

    /**
     * Stream der Ressource wird geschlossen
     */
    LIO_OP_CLOSE = 15,

    /**
     * Prüfen, ob die Ressource als Pipequelle benutzt werden kann
     */
    LIO_OP_PROBE = 16,
};

/// Flags fuer Operationen
enum lio_op_flags {
    /**
     * Die Operation gehoert dem Userspace, sobald sie fertig abgearbeitet ist,
     * muss das Flag geloescht werden. Nachdem es geloescht worden ist, darf die
     * Operation nicht mehr veraendert werden vom Userspace
     */
    LIO_OPFL_USP = 1,

    /**
     * Dieses Flag soll gesetzt werden, wenn beim abarbeiten der Operation ein
     * Fehler aufgetreten ist.
     */
    LIO_OPFL_ERROR = 2,

    /**
     * Die Operation wird momentan benutzt. Sobald dieses Flag geloescht wird,
     * darf die Operation wieder neu benutzt werden.
     */
    LIO_OPFL_USED = 4,
};

struct lio_op {
    /// Opcode dieser Operation
    int opcode;

    /**
     * Flags
     * @see enum lio_op_flags
     */
    volatile uint32_t flags;
} __attribute__((packed));

struct lio_op_load_root {
    struct lio_op op;

    /// Quellhandle fuer Dateisystem oder -1 wenn keins vorhanden
    lio_usp_stream_t source;

    lio_usp_tree_t tree;

    /// Ressourcennummer der Root-Ressource (rueckgabe)
    lio_usp_resource_t result;
} __attribute__((packed));

struct lio_op_load_children {
    struct lio_op op;

    /// Nummer der Ressource deren Kindressourcen geladen werden sollen
    lio_usp_resource_t resource;

    /// Wert des Opaque-Felds der betreffenden Ressource
    void* opaque;
} __attribute__((packed));

struct lio_op_close {
    struct lio_op op;

    /// Nummer der Ressource, die geschlossen wird
    lio_usp_resource_t resource;

    /// Wert des Opaque-Felds der betreffenden Ressource
    void* opaque;
} __attribute__((packed));

struct lio_op_read {
    struct lio_op op;

    /// Nummer der Ressource deren Datenbloecke eingelesen werden sollen
    lio_usp_resource_t resource;

    /// Wert des Opaque-Felds der betreffenden Ressource
    void* opaque;

    /// Offset von dem an gelesen werden soll
    uint64_t offset;

    /// Anzahl der zu lesenden Bytes
    size_t size;

    /// Userspace-Puffer, in den gelesen werden soll
    void* buf;
} __attribute__((packed));

struct lio_op_write {
    struct lio_op op;

    /// Nummer der Ressource in die Datenblocke geschrieben werden sollen
    lio_usp_resource_t resource;

    /// Groesse der Ressource
    uint64_t res_size;

    /// Wert des Opaque-Felds der betreffenden Ressource
    void* opaque;

    /// Offset an den geschrieben werden soll
    uint64_t offset;

    /// Anzahl der zu schreibenden Bytes
    size_t size;
    //
    /// Userspace-Puffer, der geschrieben werden soll
    void* buf;
} __attribute__((packed));

struct lio_op_make_file {
    struct lio_op op;

    /// Nummer der Elternressource
    lio_usp_resource_t parent;

    /// Wert des Opaque-Feldes der Elternressource
    void* opaque;

    /// Ressourcennummer der neu erstellten ressource
    lio_usp_resource_t result;

    /// Name der Datei (garantiert \0 terminiert)
    char name[];
} __attribute__((packed));

struct lio_op_make_dir {
    struct lio_op op;

    /// Nummer der Elternressource
    lio_usp_resource_t parent;

    /// Wert des Opaque-Feldes der Elternressource
    void* opaque;

    /// Ressourcennummer der neu erstellten ressource
    lio_usp_resource_t result;

    /// Name der Datei (garantiert \0 terminiert)
    char name[];
} __attribute__((packed));

struct lio_op_make_symlink {
    struct lio_op op;

    /// Nummer der Elternressource
    lio_usp_resource_t parent;

    /// Wert des Opaque-Feldes der Elternressource
    void* opaque;

    /// Laenge des gewuenschten Namens inklusive 0-Byte
    size_t name_len;

    /// Ressourcennummer der neu erstellten ressource
    lio_usp_resource_t result;

    /// Name des Symlinks und bei strings + name_len das target
    char strings[];
} __attribute__((packed));


struct lio_op_truncate {
    struct lio_op op;

    /// Nummer der Ressource deren Kindressourcen geladen werden sollen
    lio_usp_resource_t resource;

    /// Wert des Opaque-Felds der betreffenden Ressource
    void* opaque;

    /// Neue Dateigroesse
    uint64_t size;
} __attribute__((packed));

struct lio_op_map {
    struct lio_op op;

    /// Nummer der Ressource deren Uebersetzungstabelle befuellt werden soll
    lio_usp_resource_t resource;

    /// Wert des Opaque-Felds der betreffenden Ressource
    void* opaque;

    /// Start des Bereichs
    uint64_t offset;

    /// Groesse des Bereichs
    uint64_t size;
} __attribute__((packed));

struct lio_op_allocate {
    struct lio_op op;

    /// Nummer der Ressource deren Blocks alloziert werden sollen
    lio_usp_resource_t resource;

    /// Wert des Opaque-Felds der betreffenden Ressource
    void* opaque;

    /// Start des Bereichs
    uint64_t offset;

    /// Groesse des Bereichs
    uint64_t size;
} __attribute__((packed));

struct lio_op_unlink {
    struct lio_op op;

    /// Nummer der Elternressource
    lio_usp_resource_t parent;

    /// Wert des Opaque-Feldes der Elternressource
    void* opaque;

    /// Name der Datei (garantiert \0 terminiert)
    char name[];
} __attribute__((packed));

struct lio_op_pipe {
    struct lio_op op;

    /// Nummer der Ressource, die geoeffnet wurde
    lio_usp_resource_t res;

    /// Wert des Opaque-Felds der betreffenden Ressource
    void* opaque;

    /// Stream fuer den Serverseitigen Zugriff auf die Pipe
    lio_usp_stream_t pipe_end;

    /// Flags des Streams
    int flags;
} __attribute__((packed));

struct lio_op_sync {
    struct lio_op op;

    /// Nummer der Ressource, die geoeffnet wurde
    lio_usp_resource_t res;

    /// Wert des Opaque-Felds der betreffenden Ressource
    void* opaque;
} __attribute__((packed));

struct lio_op_probe {
    struct lio_op op;

    /// Stream fuer die Pipequelle
    lio_usp_stream_t source;

    /// Rückgabe: Informationen über Dateisystem
    struct lio_probe_service_result* probe_data;
} __attribute__((packed));


/// Flags fuer Server-Ressourcen
enum lio_server_flags {
    LIO_SRV_READABLE = 1,
    LIO_SRV_WRITABLE = 2,
    LIO_SRV_SEEKABLE = 4,
    LIO_SRV_MOREDATA = 8,
    LIO_SRV_BROWSABLE = 16,
    LIO_SRV_CHANGEABLE = 32,
    LIO_SRV_RESOLVABLE = 64,
    LIO_SRV_RETARGETABLE = 128,
    LIO_SRV_PIPE = 256,
    LIO_SRV_TRANSLATED = 512,
    LIO_SRV_IS_TERMINAL = 1024,
};


struct lio_server_resource {
    /// Groesse der Ressource (als Datei/Symlink)
    uint64_t size;

    /// Blockgroesse
    uint32_t blocksize;

    /**
     * Flags
     * @see enum lio_server_flags
     */
    uint32_t flags;

    /// Ressourcennummer (wird vom Kernel vergeben)
    lio_usp_resource_t id;

    /// Tree-ID
    lio_usp_tree_t tree;

    /**
     * Private Daten des Services zu dieser Ressource, dieser Wert wird bei den
     * Operationen jeweils mit angegeben.
     */
    void* opaque;
} __attribute__((packed));

#endif
