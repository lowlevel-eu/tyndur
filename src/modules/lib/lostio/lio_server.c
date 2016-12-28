/*
 * Copyright (c) 2010 The tyndur Project. All rights reserved.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>

#include <collections.h>
#include <syscall.h>
#include <lostio.h>

/** Ausstehende Operationen dieses Trees abarbeiten. */
static void tree_dispatch(struct lio_tree* tree);

/** Operation verarbeiten */
static void process_op(struct lio_service* service, struct lio_op* op);

// Operationen
static void op_probe(struct lio_service* service, struct lio_op* op);
static void op_load_root(struct lio_service* service, struct lio_op* op);
static void op_load_children(struct lio_service* service, struct lio_op* op);
static void op_close(struct lio_service* service, struct lio_op* op);
static void op_read(struct lio_service* service, struct lio_op* op);
static void op_write(struct lio_service* service, struct lio_op* op);
static void op_make_file(struct lio_service* service, struct lio_op* op);
static void op_make_dir(struct lio_service* service, struct lio_op* op);
static void op_make_symlink(struct lio_service* service, struct lio_op* op);
static void op_truncate(struct lio_service* service, struct lio_op* op);
static void op_map(struct lio_service* service, struct lio_op* op);
static void op_allocate(struct lio_service* service, struct lio_op* op);
static void op_unlink(struct lio_service* service, struct lio_op* op);
static void op_pipe(struct lio_service* service, struct lio_op* op);
static void op_sync(struct lio_service* service, struct lio_op* op);


static list_t* services = NULL;

void lio_dispatch(void)
{
    struct lio_service* s;
    int i;

    for (i = 0; (s = list_get_element_at(services, i)); i++) {
        void* buf;
        struct lio_op* op;
        ssize_t size;

        while (1) {
            size = ring_read_ptr(s->ring, &buf);
            if (size <= 0) {
                break;
            }

            op = buf;
            switch (op->opcode) {
                case LIO_OP_NOP:
                    lio_srv_op_done(op, 0, 0);
                    break;

                case LIO_OP_LOAD_ROOT:
                    op_load_root(s, op);
                    break;

                case LIO_OP_PROBE:
                    op_probe(s, op);
                    break;

                default:
                    printf("lio_dispatch: Fehler, unerwarteter Opcode 0x%x\n",
                        op->opcode);
                    op->flags |= LIO_OPFL_ERROR;
                    break;
            }

            ring_read_complete(s->ring);
        }
    }
}

/**
 * Ausstehende Operationen dieses Trees abarbeiten.
 */
static void tree_dispatch(struct lio_tree* tree)
{
    void* buf;
    struct lio_op* op;
    ssize_t size;

    while (1) {
        while (1) {
            size = ring_read_ptr(tree->ring, &buf);
            if (size <= 0) {
                break;
            }

            op = buf;
            process_op(tree->service, op);

            ring_read_complete(tree->ring);
        }
        lio_srv_wait();
    }
}

/**
 * Operation verarbeiten
 */
static void process_op(struct lio_service* service, struct lio_op* op)
{
    switch (op->opcode) {
        case LIO_OP_NOP:
            lio_srv_op_done(op, 0, 0);
            break;

        case LIO_OP_LOAD_CHILDREN:
            op_load_children(service, op);
            break;

        case LIO_OP_CLOSE:
            op_close(service, op);
            break;

        case LIO_OP_READ:
            op_read(service, op);
            break;

        case LIO_OP_WRITE:
            op_write(service, op);
            break;

        case LIO_OP_MAKE_FILE:
            op_make_file(service, op);
            break;

        case LIO_OP_MAKE_DIR:
            op_make_dir(service, op);
            break;

        case LIO_OP_MAKE_SYMLINK:
            op_make_symlink(service, op);
            break;

        case LIO_OP_TRUNCATE:
            op_truncate(service, op);
            break;

        case LIO_OP_MAP:
            op_map(service, op);
            break;

        case LIO_OP_ALLOCATE:
            op_allocate(service, op);
            break;

        case LIO_OP_UNLINK:
            op_unlink(service, op);
            break;

        case LIO_OP_PIPE:
            op_pipe(service, op);
            break;

        case LIO_OP_SYNC:
            op_sync(service, op);
            break;

        default:
            printf("process_op: Fehler, unerwarteter Opcode 0x%x\n",
                op->opcode);
            op->flags |= LIO_OPFL_ERROR;
            break;
    }

    op->flags &= ~LIO_OPFL_USP;
}

static void op_probe(struct lio_service* service, struct lio_op* op)
{
    struct lio_op_probe* opr = (struct lio_op_probe*) op;
    int ret = -EINVAL;

    if (service->lio_ops.probe) {
        ret = service->lio_ops.probe(service, opr->source, opr->probe_data);
    }
    lio_srv_op_done(op, ret, 0);
}

static void op_load_root(struct lio_service* service, struct lio_op* op)
{
    struct lio_op_load_root* olr = (struct lio_op_load_root*) op;
    struct lio_resource* res;
    struct lio_tree* tree;

    tree = calloc(sizeof(*tree), 1);
    tree->service = service;
    tree->source = olr->source;
    tree->id = olr->tree;

    res = service->lio_ops.load_root(tree);

    if (res) {
        void* buf;
        size_t buf_size = 4096;

        buf = mem_allocate(buf_size, 0);
        memset(buf, 0, buf_size);
        tree->ring = ring_init_reader(buf, buf_size);

        // FIXME: Den richtigen Namen des Root-Knotens brauchen wir hier noch
        // vom Kernel
        lio_node_create(NULL, res, "/");
        tree->service_thread = create_thread((uint32_t) tree_dispatch, tree);

        lio_srv_tree_set_ring(tree->id, tree->service_thread, buf, buf_size);
        lio_srv_op_done(op, 0, res->server.id);
    } else {
        free(tree);
        lio_srv_op_done(op, -EIO, 0);
    }
}

static void op_load_children(struct lio_service* service, struct lio_op* op)
{
    struct lio_op_load_children* olc = (struct lio_op_load_children*) op;
    struct lio_resource* res = (struct lio_resource*) olc->opaque;
    int ret;

    ret = service->lio_ops.load_children(res);
    lio_srv_op_done(op, ret, 0);
}

static void op_close(struct lio_service* service, struct lio_op* op)
{
    struct lio_op_close* oc = (struct lio_op_close*) op;
    struct lio_resource* res = (struct lio_resource*) oc->opaque;

    if (service->lio_ops.close) {
        service->lio_ops.close(res);
    }
    lio_srv_op_done(op, 0, 0);
}

static void op_read(struct lio_service* service, struct lio_op* op)
{
    struct lio_op_read* or = (struct lio_op_read*) op;
    struct lio_resource* res = (struct lio_resource*) or->opaque;
    int ret;

    ret = service->lio_ops.read(res, or->offset, or->size, or->buf);
    lio_srv_op_done(op, ret, 0);
}

static void op_write(struct lio_service* service, struct lio_op* op)
{
    struct lio_op_write* ow = (struct lio_op_write*) op;
    struct lio_resource* res = (struct lio_resource*) ow->opaque;
    int ret;

    if (res->size != ow->res_size) {
        service->lio_ops.resize(res, ow->res_size);
    }

    ret = service->lio_ops.write(res, ow->offset, ow->size, ow->buf);
    lio_srv_op_done(op, ret, 0);
}

static void op_make_file(struct lio_service* service, struct lio_op* op)
{
    struct lio_op_make_file* omf = (struct lio_op_make_file*) op;
    struct lio_resource* parent = (struct lio_resource*) omf->opaque;
    struct lio_resource* res;

    res = service->lio_ops.make_file(parent, omf->name);

    if (res == NULL) {
        lio_srv_op_done(op, -EIO, 0);
    } else {
        lio_srv_op_done(op, 0, res->server.id);
    }
}

static void op_make_dir(struct lio_service* service, struct lio_op* op)
{
    struct lio_op_make_dir* omd = (struct lio_op_make_dir*) op;
    struct lio_resource* parent = (struct lio_resource*) omd->opaque;
    struct lio_resource* res;

    res = service->lio_ops.make_dir(parent, omd->name);

    if (res == NULL) {
        lio_srv_op_done(op, -EIO, 0);
    } else {
        lio_srv_op_done(op, 0, res->server.id);
    }
}

static void op_make_symlink(struct lio_service* service, struct lio_op* op)
{
    struct lio_op_make_symlink* oms = (struct lio_op_make_symlink*) op;
    struct lio_resource* parent = (struct lio_resource*) oms->opaque;
    struct lio_resource* res;

    res = service->lio_ops.make_symlink(parent, oms->strings,
        oms->strings + oms->name_len);

    if (res == NULL) {
        lio_srv_op_done(op, -EIO, 0);
    } else {
        lio_srv_op_done(op, 0, res->server.id);
    }
}

static void op_truncate(struct lio_service* service, struct lio_op* op)
{
    struct lio_op_truncate* ot = (struct lio_op_truncate*) op;
    struct lio_resource* res = (struct lio_resource*) ot->opaque;
    int ret;

    if (service->lio_ops.resize) {
        ret = service->lio_ops.resize(res, ot->size);
    } else {
        ret = -EINVAL;
    }

    lio_srv_op_done(op, ret, 0);
}

static void op_map(struct lio_service* service, struct lio_op* op)
{
    struct lio_op_map* om = (struct lio_op_map*) op;
    struct lio_resource* res = (struct lio_resource*) om->opaque;
    int ret;

    if (service->lio_ops.map) {
        ret = service->lio_ops.map(res, om->offset, om->size);
    } else {
        ret = -EINVAL;
    }

    lio_srv_op_done(op, ret, 0);
}

static void op_allocate(struct lio_service* service, struct lio_op* op)
{
    struct lio_op_allocate* oa = (struct lio_op_allocate*) op;
    struct lio_resource* res = (struct lio_resource*) oa->opaque;
    int ret;

    if (service->lio_ops.allocate) {
        ret = service->lio_ops.allocate(res, oa->offset, oa->size);
    } else {
        ret = -EINVAL;
    }

    lio_srv_op_done(op, ret, 0);
}

static void op_unlink(struct lio_service* service, struct lio_op* op)
{
    struct lio_op_unlink* oul = (struct lio_op_unlink*) op;
    struct lio_resource* parent = (struct lio_resource*) oul->opaque;
    int ret;

    ret = service->lio_ops.unlink(parent, oul->name);
    lio_srv_op_done(op, ret, 0);
}

static void op_pipe(struct lio_service* service, struct lio_op* op)
{
    struct lio_op_pipe* opi = (struct lio_op_pipe*) op;
    struct lio_resource* res = (struct lio_resource*) opi->opaque;
    int ret;

    ret = service->lio_ops.pipe(res, opi->pipe_end, opi->flags);
    lio_srv_op_done(op, ret, 0);
}

static void op_sync(struct lio_service* service, struct lio_op* op)
{
    struct lio_op_sync* ops = (struct lio_op_sync*) op;
    struct lio_resource* res = (struct lio_resource*) ops->opaque;
    int ret = 0;

    if (service->lio_ops.sync) {
        ret = service->lio_ops.sync(res);
    }
    lio_srv_op_done(op, ret, 0);
}

/**
 * Neuen Service registrieren
 */
int lio_add_service(struct lio_service* service)
{
    void* buf;
    size_t buf_size = 4096;

    if (!services) {
        services = list_create();
    }

    buf = mem_allocate(buf_size, 0);
    memset(buf, 0, buf_size);
    service->ring = ring_init_reader(buf, buf_size);

    list_push(services, service);

    return lio_srv_service_register(service->name, buf, buf_size);
}

/**
 * Neue Ressource anlegen
 */
struct lio_resource* lio_resource_create(struct lio_tree* tree)
{
    struct lio_resource* res = calloc(sizeof(*res), 1);
    res->tree = tree;
    return res;
}

/**
 * Aenderungen an der Ressource publizieren
 */
void lio_resource_modified(struct lio_resource* res)
{
    res->server.size = res->size;
    res->server.blocksize = res->blocksize;
    res->server.flags =
        (res->readable ? LIO_SRV_READABLE : 0) |
        (res->writable ? LIO_SRV_WRITABLE : 0) |
        (res->seekable ? LIO_SRV_SEEKABLE : 0) |
        (res->moredata ? LIO_SRV_MOREDATA : 0) |
        (res->browsable ? LIO_SRV_BROWSABLE : 0) |
        (res->changeable ? LIO_SRV_CHANGEABLE : 0) |
        (res->resolvable ? LIO_SRV_RESOLVABLE : 0) |
        (res->retargetable ? LIO_SRV_RETARGETABLE : 0) |
        (res->ispipe ? LIO_SRV_PIPE : 0) |
        (res->translated ? LIO_SRV_TRANSLATED : 0) |
        (res->is_terminal ? LIO_SRV_IS_TERMINAL: 0);
    res->server.tree = res->tree->id;
    res->server.opaque = res;

    lio_srv_res_upload(&res->server);
}

/**
 * Neuen Knoten anlegen und bei Elternressource als Kind eintragen. Wird NULL
 * als Elternressource uebergeben, dann wird die Ressource als root in den Tree
 * eingetragen.
 */
void lio_node_create(struct lio_resource* parent, struct lio_resource* res,
    const char* name)
{
    struct lio_node* node;

    node = malloc(sizeof(*node));
    node->res = res;
    node->name = strdup(name);

    if (!parent) {
        res->tree->root = node;
    } else {
        if (!parent->children) {
            parent->children = list_create();
        }
        list_push(parent->children, node);

        lio_srv_node_add(parent->server.id, res->server.id, name);
    }

}

/**
 * Kindknoten einer Ressource suchen
 */
struct lio_node* lio_resource_get_child(struct lio_resource* parent,
    const char* name)
{
    struct lio_node* node;
    size_t i;

    for (i = 0; (node = list_get_element_at(parent->children, i)); i++) {
        if (!strcmp(node->name, name)) {
            return node;
        }
    }

    return NULL;
}

/**
 * Kindknoten einer Ressource entfernen
 */
void lio_resource_remove_child(struct lio_resource* parent,
    struct lio_node* child)
{
    struct lio_node* node;
    size_t i;

    for (i = 0; (node = list_get_element_at(parent->children, i)); i++) {
        if (node == child) {
            list_remove(parent->children, i);
            lio_srv_node_remove(parent->server.id, child->name);
            return;
        }
    }

    abort();
}
