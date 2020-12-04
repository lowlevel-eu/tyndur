/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
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

#include <stdlib.h>

#include "kernel.h"
#include "lostio/core.h"
#include "lostio/client.h"
#include "lostio_int.h"

static tree_t* resource_tree;
static tree_t* tree_tree;

/**
 * Initialisiert die Verwaltung der Userspace-Deskriptoren für Kernelobjekte
 */
int lio_init_userspace(void)
{
    resource_tree = tree_create(struct lio_resource, usp_item, usp_id);
    tree_tree = tree_create(struct lio_tree, usp_item, usp_id);
    return 0;
}

static lio_usp_resource_t new_resourceid(void)
{
    static lio_usp_resource_t prev_id = 0;
    return ++prev_id;
}

static lio_usp_stream_t new_streamid(void)
{
    static lio_usp_stream_t prev_id = 0;
    return ++prev_id;
}

static lio_usp_tree_t new_treeid(void)
{
    static lio_usp_tree_t prev_id = 0;
    return ++prev_id;
}


/** Weist einer Ressource eine Userspace-ID zu */
void lio_usp_add_resource(struct lio_resource* res)
{
    res->usp_id = new_resourceid();
    tree_insert(resource_tree, res);
}

/**
 * Gibt die zur Userspace-ID gehörende Ressource zurück, oder NULL wenn es
 * keine passende Ressource gibt.
 */
struct lio_resource* lio_usp_get_resource(lio_usp_resource_t id)
{
    return tree_search(resource_tree, id);
}


/** Gibt die Userspace-ID der Ressource wieder frei */
void lio_usp_remove_resource(struct lio_resource* res)
{
    tree_remove(resource_tree, res);
}

/** Weist einem Stream eine Userspace-ID zu */
struct lio_usp_stream* lio_usp_do_add_stream(pm_process_t* proc,
    struct lio_stream* stream, lio_usp_stream_t id)
{
    struct lio_usp_stream* entry = malloc(sizeof(*entry));

    entry->stream = stream;
    entry->usp_id = id;
    entry->origin = proc->pid;
    tree_insert(proc->lio_streams, entry);

    stream->usp_refcount++;

    return entry;
}

/** Weist einem Stream eine Userspace-ID zu */
struct lio_usp_stream* lio_usp_add_stream(pm_process_t* proc,
    struct lio_stream* stream)
{
    return lio_usp_do_add_stream(proc, stream, new_streamid());
}

/**
 * Gibt den zur Userspace-ID gehörenden Stream zurück, oder NULL wenn es
 * keinen passenden Stream gibt.
 */
struct lio_usp_stream* lio_usp_get_stream(pm_process_t* proc,
    lio_usp_stream_t id)
{
    return tree_search(proc->lio_streams, id);
}

/**
 * Gibt die Userspace-ID des Streams wieder frei und schließt den Stream, wenn
 * das die letzte Userspace-Referenz war.
 */
int lio_usp_unref_stream(pm_process_t* proc, struct lio_usp_stream* us)
{
    struct lio_stream* stream = us->stream;
    int ret;

    BUG_ON(stream->usp_refcount <= 0);
    stream->usp_refcount--;

    if (stream->usp_refcount == 0) {
        ret = lio_close(stream);
        if (ret < 0) {
            return ret;
        }
    }

    tree_remove(proc->lio_streams, us);
    free(us);

    return 0;
}

/** Weist einem Verzeichnisbaum eine Userspace-ID zu */
void lio_usp_add_tree(struct lio_tree* tree)
{
    tree->usp_id = new_treeid();
    tree_insert(tree_tree, tree);
}

/**  Gibt die Userspace-ID des Verzeichnisbaums wieder frei */
void lio_usp_remove_tree(struct lio_tree* tree)
{
    tree_remove(tree_tree, tree);
}

/**
 * Gibt den zur Userspace-ID gehörenden Verzeichnisbaum zurück, oder NULL
 * wenn es keinen passenden Verzeichnisbaum gibt.
 */
struct lio_tree* lio_usp_get_tree(lio_usp_tree_t id)
{
    return tree_search(tree_tree, id);
}

/**
 * Gitb die Userspace-ID zu einer gegebenen Ressource zurück.
 */
lio_usp_resource_t lio_usp_get_id(struct lio_resource* res)
{
    return res->usp_id;
}
