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
/** \addtogroup LostIO
 *  @{
 */

#ifndef _LOSTIO_H_
#define _LOSTIO_H_

#include "types.h"
#include "collections.h"
#include "io.h"

#include <stdint.h>
#include <stdbool.h>
#include <syscall.h> /* lio_*_t */


///ID des Verzeichnis Typs
#define LOSTIO_TYPES_DIRECTORY  1

///ID des Ramfile Typs
#define LOSTIO_TYPES_RAMFILE    128


#define LOSTIO_FLAG_EOF 0x10000
#define LOSTIO_FLAG_BROWSABLE 0x20000
///Der Knoten ist ein Symlink.
#define LOSTIO_FLAG_SYMLINK 0x40000
/// EOF nicht automatisch setzen wenn beim Oeffnen die Groesse 0 ist
#define LOSTIO_FLAG_NOAUTOEOF 0x80000
/// Readahead erlauben (d.h. ein zweites read muss dieselben Daten liefern)
#define LOSTIO_FLAG_READAHEAD 0x100000

/*#define LOSTIO_MODE_READ 0x1
#define LOSTIO_MODE_WRITE 0x2
#define LOSTIO_MODE_APPEND 0x4
#define LOSTIO_MODE_BROWSE 0x8
*/
///Typ der Typehandle-IDs
typedef uint8_t typeid_t;

///Knoten im VFS-Baum
typedef struct vfstree_node_t
{
    typeid_t        type;
    char*           name;
    uint64_t        size;
    void*           data;
    uint64_t        resid;
    uint32_t        flags;

    list_t*         children;
    struct vfstree_node_t* parent;
} vfstree_node_t;



///Handle fuer eine geoeffnete Datei
#ifndef __FILE_DEFINED
#define __FILE_DEFINED
typedef struct lostio_internal_file FILE;
#endif

typedef struct
{
    uint32_t        id;
    pid_t           pid;
    uint32_t        flags;
    struct lostio_internal_file*           source;
    uint64_t        pos;
    
    ///Modulspezifische Daten
    void*           data;
    
    vfstree_node_t* node;
} lostio_filehandle_t;


///Rueckgabe der 
typedef struct
{
    void*           data;
    size_t          size;
} read_hdl_reply;


///Handle um einen bestimmten Knotentyp zu beschreiben
typedef struct
{
    typeid_t        id;
    bool            (*not_found)(char**, uint8_t, pid_t,struct lostio_internal_file*);
    bool            (*pre_open)(char**, uint8_t, pid_t,struct lostio_internal_file*);
    void            (*post_open)(lostio_filehandle_t*);

    size_t          (*read)(lostio_filehandle_t*,void*,size_t,size_t);
    size_t          (*readahead)(lostio_filehandle_t*,void*,size_t,size_t);
    size_t          (*write)(lostio_filehandle_t*,size_t,size_t,void*);
    int             (*seek)(lostio_filehandle_t*,uint64_t,int);
    int             (*close)(lostio_filehandle_t*);
    int             (*link)(lostio_filehandle_t*,lostio_filehandle_t*,
                        const char*);
    int             (*unlink)(lostio_filehandle_t*,const char*);
} typehandle_t;


/// Geoeffnete Datei auf Clientseite (Wrapper fuer FILE)
struct lostio_internal_file {
    io_resource_t*  res;
    size_t          ungetc_count;
    uint8_t*        ungetc_buffer;

    void*           buffer_ptr;
    size_t          buffer_size;
    size_t          buffer_pos;
    size_t          buffer_filled;
    uint8_t         buffer_mode;
    bool            buffer_writes;
    bool            free_buffer;

    uint64_t        cur_pos;

    /*
     * Achtung: os_eof ist _nicht_ der EOF-Marker aus der C-Spezifikation, der
     * erst gesetzt wird, wenn das Programm aus dem Puffer das letzte Byte
     * einliest, sondern das EOF, auf das beim Einlesen des Puffers gestoßen
     * wurde!
     */
    bool            os_eof;
    bool            error;
};
#define IS_LIO2(h) ((h)->lio2_res > 0)

lio_stream_t file_get_stream(FILE *io_res);
bool file_is_terminal(FILE* io_res);

///LostIO-Schnittstelle initialisieren
void lostio_init(void);

///LostIO-Interne vorgaenge abarbeiten
void lostio_dispatch(void);

///Typehandle in die Liste einfuegen
void lostio_register_typehandle(typehandle_t* typehandle);

///Typehandle anhand der ID finden
typehandle_t* get_typehandle(typeid_t id);



///Neuen Knoten im VFS-Baum erstellen
bool vfstree_create_node(char* path, typeid_t type, size_t size, void* data, uint32_t flags);

///Neuen Kindknoten erstellen
bool vfstree_create_child(vfstree_node_t* parent, char* name, typeid_t type, size_t size, void* data, uint32_t flags);

///Knoten aus dem VFS-Baum loeschen
bool vfstree_delete_node(char* path);

///Kindknoten loeschen
bool vfstree_delete_child(vfstree_node_t* parent, const char* name);

///
void vfstree_clear_node(vfstree_node_t* node);

///Dateinamen aus einem ganzen Pfad extrahieren
char* vfstree_basename(char* path);

///Verzeichnisnamen aus einem Pfad extrahieren
char* vfstree_dirname(char* path);

///Pointer auf einen Knoten anhand des Elternknotens und es Namens ermitteln
vfstree_node_t* vfstree_get_node_by_name(vfstree_node_t* parent, char* name);

///Pointer auf einen Knoten anhand seines Pfades ermitteln
vfstree_node_t* vfstree_get_node_by_path(char* path);



///Den Ramfile-Typ benutzbar machen
void lostio_type_ramfile_use(void);

///Den Ramfile-Typ unter einer bestimmten ID benutzbar machen
void lostio_type_ramfile_use_as(typeid_t id);

///Den Verzechnis-Typ benutzbar machen
void lostio_type_directory_use(void);

///Den Verzechnis-Typ unter einer bestimmten ID benutzbar machen
void lostio_type_directory_use_as(typeid_t id);


/// Stream oeffnen
io_resource_t* lio_compat_open(const char* filename, uint8_t attr);

/// Stream schliessen
int lio_compat_close(io_resource_t* io_res);

/**
 * Liest aus einer Ressource
 *
 * @return Anzahl gelesener Bytes; 0 bei Dateiende; negativ im Fehlerfall.
 */
ssize_t lio_compat_read(void* dest, size_t blocksize, size_t blockcount,
    io_resource_t* io_res);

/** Liest aus einer Ressource ohne zu blockieren. */
ssize_t lio_compat_read_nonblock(void* dest, size_t size,
                                 io_resource_t* io_res);

/** Liest aus einer Ressource, ohne den Dateizeiger zu verändern */
ssize_t lio_compat_readahead(void* dest, size_t size, io_resource_t* io_res);

/**
 * Schreibt in eine Ressource
 *
 * @return Anzahl geschriebener Bytes; negativ im Fehlerfall.
 */
ssize_t lio_compat_write(const void* src, size_t blocksize, size_t blockcount,
    io_resource_t* io_res);

/// Prueft, ob das Ende des Streams erreicht ist
int lio_compat_eof(io_resource_t* io_res);

/// Stream-Position setzen
bool lio_compat_seek(io_resource_t* io_res, uint64_t offset, int origin);

/// Stream-Position abfragen
int64_t lio_compat_tell(io_resource_t* io_res);


/******************************************************************************
 * LIO2-Server                                                                *
 ******************************************************************************/

/**
 * LIO-Operationen durchfuehren, falls pendente vorhanden sind.
 */
void lio_dispatch(void);

struct lio_service;
struct lio_tree;
struct lio_driver {
    /// Prüfen, ob die Ressource als Pipequelle benutzt werden kann
    int (*probe)(struct lio_service* service,
                 lio_stream_t source,
                 struct lio_probe_service_result* probe_data);

    /// Root-Ressource eines Baums laden
    struct lio_resource* (*load_root)(struct lio_tree* tree);

    /// Kindressoucren einer Ressource laden
    int (*load_children)(struct lio_resource* res);

    /// Stream der Ressource wurde geschlossen
    void (*close)(struct lio_resource* res);

    /// Daten aus einer Ressource in den Cache laden
    int (*read)(struct lio_resource* res, uint64_t offset,
        size_t bytes, void* buf);

    /// Daten aus dem Cache in eine Ressource zurückschreiben
    int (*write)(struct lio_resource* res, uint64_t offset,
        size_t bytes, void* buf);

    /// Groesse einer Ressource aendern
    int (*resize)(struct lio_resource* res, uint64_t size);

    /// Neue Datei erstellen
    struct lio_resource* (*make_file)(struct lio_resource* parent,
        const char* name);

    /// Neues Verzeichnis erstellen
    struct lio_resource* (*make_dir)(struct lio_resource* parent,
        const char* name);

    /// Neuen Symlink erstellen
    struct lio_resource* (*make_symlink)(struct lio_resource* parent,
        const char* name, const char* target);

    /// Blocktabelle einlesen
    int (*map)(struct lio_resource* res, uint64_t offset, uint64_t size);

    /// Sparse-Blocks allozieren
    int (*allocate)(struct lio_resource* res, uint64_t offset, uint64_t size);

    /// Verzeichniseintrag loeschen
    int (*unlink)(struct lio_resource* parent, const char* name);

    /// Neue Pipe erstellen
    int (*pipe)(struct lio_resource* res, lio_stream_t pipe_end, int flags);

    /// Caches zurückschreiben
    int (*sync)(struct lio_resource* res);
};

struct lio_service {
    const char*         name;
    struct lio_driver   lio_ops;
    void*               opaque;

    // Intern
    struct shared_ring* ring;
};

struct lio_node;
struct lio_tree {
    struct lio_node*    root;
    struct lio_service* service;
    lio_stream_t        source;
    void*               opaque;

    // Intern
    lio_usp_tree_t      id;
    tid_t               service_thread;
    struct shared_ring* ring;
};

struct lio_resource {
    list_t* children;
    uint64_t size;
    uint32_t blocksize;

    bool readable;
    bool writable;
    bool seekable;
    bool moredata;
    bool browsable;
    bool changeable;
    bool resolvable;
    bool retargetable;
    bool ispipe;
    bool translated;
    bool is_terminal;

    struct lio_tree* tree;
    void* opaque;

    // Intern
    struct lio_server_resource server;
};

struct lio_node {
    struct lio_resource* res;
    char* name;
};

/**
 * Neuen Service registrieren
 */
int lio_add_service(struct lio_service* service);


/**
 * Neue Ressource anlegen
 */
struct lio_resource* lio_resource_create(struct lio_tree* tree);

/**
 * Aenderungen an der Ressource publizieren
 */
void lio_resource_modified(struct lio_resource* res);

/**
 * Kindknoten einer Ressource suchen
 */
struct lio_node* lio_resource_get_child(struct lio_resource* parent,
    const char* name);

/**
 * Kindknoten einer Ressource entfernen
 */
void lio_resource_remove_child(struct lio_resource* parent,
    struct lio_node* child);

/**
 * Neuen Knoten anlegen und bei Elternressource als Kind eintragen. Wird NULL
 * als Elternressource uebergeben, dann wird die Ressource als root in den Tree
 * eingetragen.
 */
void lio_node_create(struct lio_resource* parent, struct lio_resource* res,
    const char* name);

/**
 * Uebersetzte Cache-Blocks anfordern
 */
int lio_request_translated_blocks(struct lio_resource* res, uint64_t offset,
    size_t num, uint64_t* translation_table);


/** @}  */

/* Interne Funktionen */
int __iores_fileno(io_resource_t* io_res);
struct lostio_internal_file* __create_file_from_iores(io_resource_t* io_res);
struct lostio_internal_file* __create_file_from_lio_stream(lio_stream_t s);

#endif

