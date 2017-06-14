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

#include <stdbool.h>
#include <stdlib.h>
#include <syscall.h>
#include <string.h>
#include <errno.h>

#include "vterm.h"
#include "lostio.h"
#include "keymap.h"

static struct lio_resource* root;

static void lio_add_vterm_dir(vterminal_t* vterm)
{
    struct lio_tree* tree = root->tree;

    struct lio_resource* dir;
    struct lio_resource* in_node;
    struct lio_resource* out_node;

    struct lio_vterm* lv;

    // Verzeichnis
    dir = lio_resource_create(tree);
    dir->browsable = 1;
    dir->blocksize = 1;
    lio_resource_modified(dir);
    lio_node_create(root, dir, vterm->name);

    // In-Node
    lv = malloc(sizeof(*lv));
    *lv = (struct lio_vterm) {
        .type   = LOSTIO_TYPES_IN,
        .vterm  = vterm,
    };

    in_node = lio_resource_create(tree);
    in_node->readable = 1;
    in_node->blocksize = 1;
    in_node->moredata = 1;
    in_node->is_terminal = true;
    in_node->opaque = lv;
    lio_resource_modified(in_node);
    lio_node_create(dir, in_node, "in");

    vterm->lio_in_res = in_node;

    // Out-Node
    lv = malloc(sizeof(*lv));
    *lv = (struct lio_vterm) {
        .type   = LOSTIO_TYPES_OUT,
        .vterm  = vterm,
    };

    out_node = lio_resource_create(tree);
    out_node->writable = 1;
    out_node->blocksize = 1;
    out_node->is_terminal = true;
    out_node->opaque = lv;
    lio_resource_modified(out_node);
    lio_node_create(dir, out_node, "out");
}

struct lio_resource* vterm_load_root(struct lio_tree* tree)
{
    vterminal_t *vterm;
    struct lio_resource* keymap_node;
    struct lio_vterm* lv;
    int i;

    if (tree->source != -1) {
        return NULL;
    }

    // Wurzelknoten
    root = lio_resource_create(tree);
    root->browsable = 1;
    root->blocksize = 1;
    lio_resource_modified(root);
    lio_node_create(NULL, root, "");

    // keymap
    lv = malloc(sizeof(*lv));
    *lv = (struct lio_vterm) {
        .type   = LOSTIO_TYPES_KEYMAP,
        .vterm  = NULL,
    };

    keymap_node = lio_resource_create(tree);
    keymap_node->readable = 1;
    keymap_node->writable = 1;
    keymap_node->blocksize = 1;
    keymap_node->size = sizeof(keymap);
    keymap_node->opaque = lv;
    lio_resource_modified(keymap_node);
    lio_node_create(root, keymap_node, "keymap");

    // Terminals
    for (i = 0; (vterm = list_get_element_at(vterm_list, i)); i++) {
        lio_add_vterm_dir(vterm);
    }

    return root;
}

void lio_add_vterm(vterminal_t* vterm)
{
    if (root) {
        lio_add_vterm_dir(vterm);
    }
}

/**
 * LostIO-Handler um in den Ausgabekanal zu schreiben
 */
int term_write(struct lio_resource* res, uint64_t offset, size_t bytes,
               void* buf)
{
    struct lio_vterm *lv = res->opaque;

    //printf("term_write: %s: %d -> %p\n", lv->vterm->name, bytes, buf);

    p();
    vterm_process_output(lv->vterm, buf, bytes);
    v();

    return 0;
}

/**
 * LostIO-Handller um aus dem Eingabekanal zu lesen
 */
int term_read(struct lio_resource* res, uint64_t offset, size_t bytes,
               void* buf)
{
    struct lio_vterm *lv = res->opaque;

    p();
    vterminal_t* vterm = lv->vterm;

    /*printf("vterm_read: offset %llx + %d bytes [buf %d; res %d; seteof %d; "
           "moredata %d]\n", offset, bytes,
           vterm->in_buffer_size, res->size, vterm->set_eof, res->moredata);*/

    // FIXME Erstmal die vorherigen Daten rausgeben
    if (vterm->set_eof) {
        vterm->set_eof = false;
        res->moredata = false;
        lio_resource_modified(res);
        v();
        return -EAGAIN;
    } else if (res->moredata == false) {
        res->moredata = true;
        lio_resource_modified(res);
    }

    // Erst einmal alles wegwerfen, was nicht mehr gebraucht wird, weil schon
    // von einem höheren Offset gelesen wurde
    if (offset > vterm->in_buffer_offset) {
        size_t drop;

        drop = offset - vterm->in_buffer_offset;
        if (drop > vterm->in_buffer_size) {
            drop = vterm->in_buffer_size;
        }
        vterm->in_buffer_size -= drop;

        if (vterm->in_buffer_size != 0) {
            memmove(vterm->in_buffer, vterm->in_buffer + drop,
                    vterm->in_buffer_size);
        } else {
            free(vterm->in_buffer);
            vterm->in_buffer = NULL;
        }

        vterm->in_buffer_offset = offset;
    }

    if (vterm->in_buffer_size < bytes) {
        v();
        return -EAGAIN;
    }

/*printf("vterm_read: memcpy %p <- %p + %x [buf %x]\n",
    buf, vterm->in_buffer, bytes, vterm->in_buffer_size);*/
    memcpy(buf, vterm->in_buffer, bytes);

    v();
    return 0;
}

void lio_update_in_file_size(vterminal_t* vterm, size_t length)
{
    struct lio_resource* res = vterm->lio_in_res;

    if (res) {
        res->size += length;
        lio_resource_modified(res);
    }
}

void lio_set_eof(vterminal_t* vterm)
{
    struct lio_resource* res = vterm->lio_in_res;

    if (res) {
        res->moredata = false;
        lio_resource_modified(res);
    }
}

int vterm_read(struct lio_resource* res, uint64_t offset, size_t bytes,
                void* buf)
{
    struct lio_vterm *lv = res->opaque;

    switch (lv->type) {
        case LOSTIO_TYPES_IN:       return term_read(res, offset, bytes, buf);
        case LOSTIO_TYPES_OUT:      return -EBADF;
        case LOSTIO_TYPES_KEYMAP:   return keymap_read(offset, bytes, buf);
    }

    return -EBADF;
}

int vterm_write(struct lio_resource* res, uint64_t offset, size_t bytes,
                void* buf)
{
    struct lio_vterm *lv = res->opaque;

    switch (lv->type) {
        case LOSTIO_TYPES_IN:       return -EBADF;
        case LOSTIO_TYPES_OUT:      return term_write(res, offset, bytes, buf);
        case LOSTIO_TYPES_KEYMAP:   return keymap_write(offset, bytes, buf);
    }

    return -EBADF;
}

int vterm_resize(struct lio_resource* res, uint64_t size)
{
    struct lio_vterm *lv = res->opaque;

    switch (lv->type) {
        case LOSTIO_TYPES_IN:       return 0;
        case LOSTIO_TYPES_OUT:      return 0;
        case LOSTIO_TYPES_KEYMAP:   return 0; /* TODO -EINVAL; */
    }

    return -EBADF;
}

void vterm_close(struct lio_resource* res)
{
    struct lio_vterm *lv = res->opaque;

    if (lv->type == LOSTIO_TYPES_IN) {
        lv->vterm->in_buffer_offset = 0;
        res->size = lv->vterm->in_buffer_size = 0;
        lio_resource_modified(res);
    }
}

static struct lio_service service = {
    .name               = "vterm",
    .lio_ops = {
        .load_root      = vterm_load_root,
        .close          = vterm_close,
        .read           = vterm_read,
        .write          = vterm_write,
        .resize         = vterm_resize,
    },
};

/**
 * LostIO-Interface vorbereiten, ueber das die Prozesse mit vterm kommunizieren
 * werden.
 */
void init_lostio_interface()
{
    // TODO Entfernen, wenn niemand mehr versucht, LIOv1 zu benutzen
    // Im Moment brauchen wir das noch, damit RPCs beantwortet werden und der
    // Aufrufer nicht hängt
    lostio_init();

    lio_add_service(&service);
}
