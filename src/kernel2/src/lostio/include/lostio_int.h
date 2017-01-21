/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
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

#ifndef LOSTIO_INT_H
#define LOSTIO_INT_H

#include <collections.h>
#include <syscall_structs.h>

#include "lostio/core.h"
#include "notifier.h"
#include "tasks.h"

#define ROUND_TO_NEXT_BLOCK(bytes, blocksize) \
    ((bytes) + (blocksize) - 1) & ~((blocksize) - 1)
#define ROUND_TO_BLOCK_DOWN(bytes, blocksize) \
    ((bytes)) & ~((blocksize) - 1)

struct lio_service;
struct lio_driver {
    /** Prüfen, ob die Ressource als Pipequelle benutzt werden kann */
    int (*probe)(struct lio_service* service, struct lio_stream* source,
                 struct lio_probe_service_result* probe_data);

    /**
     * Root-Ressource eines Dateisystems laden, das root-Feld in tree ist also
     * noch NULL.
     */
    struct lio_resource* (*load_root)(struct lio_tree* tree);

    /**
     * Laedt alle Kindknoten nach res->children
     */
    int (*load_children)(struct lio_resource* res);

    /**
     * Wird aufgerufen wenn eine Ressource geoeffnet wird.
     *
     * @return 0 bei Erfolg, negativ bei Fehlern. Bei einem Fehler wird das
     * Oeffnen abgebrochen.
     */
    int (*preopen)(struct lio_stream* stream);

    /** Stream wird geschlossen */
    void (*close)(struct lio_stream* stream);

    /** Erstellt eine neue Datei */
    struct lio_resource* (*make_file)(struct lio_resource* parent,
        const char* name);

    /** Erstellt ein neues Verzeichnis */
    struct lio_resource* (*make_dir)(struct lio_resource* parent,
        const char* name);

    /** Erstellt einen neuen Symlink */
    struct lio_resource* (*make_symlink)(struct lio_resource* parent,
        const char* name, const char* target);

    /** Verzeichniseintrag loeschen */
    int (*unlink)(struct lio_resource* parent, const char* name);

    /**
     * Laedt Daten vom Service in den Cache.
     * offset und bytes muessen auf die Blockgroesse ausgerichtet sein.
     *
     * @return 0 bei Erfolg (Puffer ist komplett gelesen), negativ im
     * Fehlerfall.
     *
     * TODO iovec statt bytes/buf
     */
    int (*read)(struct lio_resource* res, uint64_t offset,
        size_t bytes, void* buf);

    /**
     * Schreibt Daten aus dem Cache in den Service zurueck
     * offset und bytes muessen auf die Blockgroesse ausgerichtet sein.
     *
     * @return 0 bei Erfolg (Puffer ist komplett geschrieben), negativ im
     * Fehlerfall.
     *
     * TODO iovec statt bytes/buf
     */
    int (*write)(struct lio_resource* res, uint64_t offset,
        size_t bytes, void* buf);

    /** Schreibt eventuelle im Treiber gecachte Daten zurueck */
    int (*sync)(struct lio_resource* res);

    /**
     * Setzt die Dateigroesse.
     *
     * Diese Funktion wird auch aufgerufen, bevor eine Datei durch einen
     * Schreibzugriff vergroessert wird. Ein Service kann write()-Aufrufe
     * hinter dem Dateiende verwerfen.
     */
    int (*truncate)(struct lio_resource* res, uint64_t size);

    /**
     * Neue Pipe erstellen.
     *
     * Wird aufgerufen wenn jemand eine Ressource oeffnet, die als pipe
     * gekennzeichnet wurde. Der Kernel erstellt dann 2 neue Ressourcen, die an
     * den beiden Enden jeweils vertauscht als Lese- und Schreibressource
     * eingetragen werden.
     */
    int (*pipe)(struct lio_resource* res, struct lio_stream* pipe_end,
        int flags);
};


/**
 * Beschreibt einen LostIO-Service.
 */
struct lio_service {
    /** Eindeutiger Name des Service */
    char*                   name;

    /** Funktionspointer, die den eigentlichen Treiber implementieren */
    struct lio_driver       lio_ops;

    /** Private Daten des Services */
    void*                   opaque;
};



/**
 * Beschreibt eine von LostIO verwaltete Ressource (d.h. in der Regel eine
 * Datei). Alle Informationen, die nicht für einen bestimmten Client
 * spezifisch sind, werden hier gespeichert.
 */
struct lio_resource {

    /** Verzeichnisbaum, zu dem die Ressource gehört */
    struct lio_tree*            tree;

    /**
     * Verzeichniseinträge (struct lio_node), falls die Ressource ein
     * Verzeichnis ist
     */
    list_t*                     children;

    /** Referenzzähler */
    int                         ref;

    /** Größe der Ressource in Bytes */
    uint64_t                    size;

    /**
     * Die Blockgröße ist die kleinste Einheit, in der Anfragen an den
     * Service gemacht werden. Sie wird auch intern für den Cache verwendet.
     * Nur Zweierpotenzen; block_size * LIO_CLUSTER_SIZE sollte durch PAGE_SIZE
     * teilbar sein.
     */
    uint32_t                    blocksize;

    /** Hat unterschiedliche Ressourcen fuer Lesen und Schreiben */
    bool                        ispipe;

    bool                        readable;
    bool                        writable;
    bool                        seekable;
    bool                        moredata;
    bool                        browsable;
    bool                        changeable;
    bool                        resolvable;
    bool                        retargetable;
    bool                        is_terminal;

    /**
     * Wird aufgerufen, wenn sich die Größe einer Ressource ändert (dies
     * schließt Fälle ein, in denen moredata von true zu false wechselt, wenn
     * dann statt -EGAIN das Dateiende gemeldet wird.
     */
    struct notifier*            on_resize;

    /** Id der Ressource fuer Userspace-Programme */
    lio_usp_resource_t          usp_id;

    /** Sortierung nach usp_id im Baum */
    struct tree_item            usp_item;

    /**
     * Falls die Ressource nur einmal geöffnet werden kann, der offene Stream,
     * der sie zugreift, oder NULL, wenn sie nicht geöffnet ist.
     */
    struct lio_stream*          excl_stream;

    /** Private Daten des Services */
    void*                       opaque;
};

struct lio_tree {
    struct lio_node*        root;
    struct lio_service*     service;
    struct lio_stream*      source;

    lio_usp_tree_t          usp_id;
    struct tree_item        usp_item;

    /** Private Daten des Services */
    void*                   opaque;
};


/**
 * Initialisiert die statischen Variablen von tree.c
 */
int lio_init_tree(void);

/**
 * Initialisiert die Verwaltung der Userspace-Deskriptoren für Kernelobjekte
 */
int lio_init_userspace(void);

/**
 * Neue Ressource initialisieren.
 * Vom Aufrufer initialisiert werden muessen (werden alle auf 0 initialisiert):
 *   - blocksize
 *   - tree
 *   - Flags
 *
 * @return Zeiger auf die neue Ressource
 */
struct lio_resource* lio_create_resource(void);

/**
 * Interne Daten einer Ressource initialisieren. Dabei werden nur die internen
 * Felder veraendert, der Rest muss vom Aufrufer gesetzt werden. Diese Funktion
 * kann verwendet werden, wenn der Speicher fuer eine Resource bereits
 * verfuegbar ist (beispielsweise bei statischem Speicher).
 *
 * @param res Zu initialisierende Ressource
 */
void lio_init_resource(struct lio_resource* res);

/**
 * Ressource zerstören und Speicher freigeben. Der Aufrufer ist dafür
 * zuständig, allenfalls Caches zu syncen.
 *
 * @param res Zu zerstoerende Ressource
 */
void lio_destroy_resource(struct lio_resource* res);

/* Pipe-Funktionen */
struct lio_resource* lio_create_pipe(void);
void lio_destroy_pipe(struct lio_resource* res);

/** Loest einen Pfad in eine Ressource auf */
struct lio_resource* lio_do_get_resource(const char* path, int follow_symlinks,
                                         int depth);

/**
 * Loest einen gegebenen Pfad in einen Verzeichnisbaum und einen
 * servicerelativen Pfad auf.
 *
 * @param path Aufzulosender Pfad
 * @param rel_path Wenn rel_path != NULL ist, wird darin ein Pointer auf den
 * servicerelativen Teil des Pfads zurueckgegeben
 *
 * @return Den zum Pfad gehoerenden Verzeichnisbaum oder NULL im Fehlerfall
 */
struct lio_tree* lio_get_tree(const char* path, const char** rel_path,
                              int depth);

/**
 * Registriert einen neuen LIO-Service
 */
void lio_add_service(struct lio_service* service);

/** Kindknoten einer Ressource suchen */
struct lio_node* lio_resource_get_child(struct lio_resource* parent,
    const char* name);

/** Einer Resource einen Kindknoten anfuegen */
void lio_resource_add_child(struct lio_resource* parent,
    struct lio_node* child);

/** Kindknoten einer Ressource entfernen */
void lio_resource_remove_child(struct lio_resource* parent,
    struct lio_node* child);

/** Neuen Knoten anlegen und bei Elternressource als Kind eintragen */
struct lio_node* lio_create_node(struct lio_resource* parent,
    struct lio_resource* res, const char* name);

/** Größe aktualisieren und on_size-Notifier ausführen */
void lio_resource_update_size(struct lio_resource* res, uint64_t size);


/** Weist einer Ressource eine Userspace-ID zu */
void lio_usp_add_resource(struct lio_resource* res);

/** Gibt die Userspace-ID der Ressource wieder frei */
void lio_usp_remove_resource(struct lio_resource* res);

/** Weist einem Verzeichnisbaum eine Userspace-ID zu */
void lio_usp_add_tree(struct lio_tree* tree);

/**  Gibt die Userspace-ID des Verzeichnisbaums wieder frei */
void lio_usp_remove_tree(struct lio_tree* tree);

#endif
