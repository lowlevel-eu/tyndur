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

/** ID eines LostIO-Streams */
typedef int64_t lio_usp_stream_t;

/** ID einer LostIO-Ressource */
typedef int64_t lio_usp_resource_t;

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

#endif
