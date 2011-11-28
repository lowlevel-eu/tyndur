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
#include "tasks.h"

struct lio_service;

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

    /** Id der Ressource fuer Userspace-Programme */
    lio_usp_resource_t          usp_id;

    /** Sortierung nach usp_id im Baum */
    struct tree_item            usp_item;

    /** Private Daten des Services */
    void*                       opaque;
};

struct lio_tree {
    struct lio_node*        root;
    struct lio_service*     service;
    struct lio_stream*      source;

    lio_usp_tree_t          usp_id;
    struct tree_item        usp_item;
};


/**
 * Initialisiert die Verwaltung der Userspace-Deskriptoren für Kernelobjekte
 */
int lio_init_userspace(void);


/** Weist einer Ressource eine Userspace-ID zu */
void lio_usp_add_resource(struct lio_resource* res);

/** Gibt die Userspace-ID der Ressource wieder frei */
void lio_usp_remove_resource(struct lio_resource* res);

/** Weist einem Verzeichnisbaum eine Userspace-ID zu */
void lio_usp_add_tree(struct lio_tree* tree);

/**  Gibt die Userspace-ID des Verzeichnisbaums wieder frei */
void lio_usp_remove_tree(struct lio_tree* tree);

#endif
