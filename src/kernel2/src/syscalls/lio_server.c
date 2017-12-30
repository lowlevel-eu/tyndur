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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "kernel.h"
#include "kprintf.h"
#include "tasks.h"
#include "lostio/userspace.h"
#include "../lostio/include/lostio_int.h"
#include "syscall.h"
#include "mm.h"

/// Eintraege in der usp_mapped_blocks Liste der Services
struct cache_block_handle {
    uint64_t offset;
    size_t num;
    void* mapped;
};

static struct lio_resource* load_root(struct lio_tree* tree);
static int load_children(struct lio_resource* res);
static void close(struct lio_stream* stream);
static int read(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf);
static int write(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf);
static int sync(struct lio_resource* res);
static struct lio_resource* make_file(struct lio_resource* parent,
    const char* name);
static struct lio_resource* make_dir(struct lio_resource* parent,
    const char* name);
static struct lio_resource* make_symlink(struct lio_resource* parent,
    const char* name, const char* target);
static int truncate(struct lio_resource* res, uint64_t size);
static int unlink(struct lio_resource* parent, const char* name);
static int probe(struct lio_service* serv, struct lio_stream* source,
                 struct lio_probe_service_result* probe_data);
static int pipe(struct lio_resource* res, struct lio_stream* pipe_end,
    int flags);

struct lio_pending_op {
    struct tree_item    tinfo;
    pm_thread_t*        caller;
    uint64_t            op_addr;
    int*                status;
    uint64_t*           result;
};

struct lio_ring {
    struct shared_ring* ring;
    uintptr_t           kern_base;
    uintptr_t           usp_base;
    tree_t*             pending;
};

struct lio_userspace_service {
    struct lio_ring     ring;

    /** Prozess des Services */
    pm_thread_t*        provider;
};

struct lio_userspace_tree {
    struct lio_ring     ring;
    pm_thread_t*        thread;
};

static void service_on_thread_destroy(pm_thread_t* thread, void* prv)
{
    struct lio_service* s;

    while ((s = list_pop(thread->lio_services))) {
        struct lio_userspace_service* us = s->opaque;
        us->provider = NULL;
    }

    list_destroy(thread->lio_services);
}

static void tree_on_thread_destroy(pm_thread_t* thread, void* prv)
{
    struct lio_tree* t;

    while ((t = list_pop(thread->lio_trees))) {
        struct lio_userspace_tree* ut = t->opaque;
        if (ut) {
            t->opaque = NULL;
            free(ut);
        }
    }

    list_destroy(thread->lio_trees);
}

/// Neuen lio-Service registrieren
int syscall_lio_srv_service_register(const char* name, size_t name_len,
    void* buffer, size_t size)
{
    struct lio_service* service;
    struct lio_userspace_service* s;
    list_t* services;
    void* buf;

    service = calloc(sizeof(*service), 1);

    // TODO: Ein paar is_userspace
    service->name = strdup(name);
    service->lio_ops.probe= probe;
    service->lio_ops.load_root = load_root;
    service->lio_ops.load_children = load_children;
    service->lio_ops.close = close;
    service->lio_ops.read = read;
    service->lio_ops.write = write;
    service->lio_ops.make_file = make_file;
    service->lio_ops.make_dir = make_dir;
    service->lio_ops.make_symlink = make_symlink;
    service->lio_ops.truncate = truncate;
    service->lio_ops.unlink = unlink;
    service->lio_ops.pipe = pipe;
    service->lio_ops.sync = sync;

    buf = mmc_automap_user(&mmc_current_context(),
        &mmc_current_context(), buffer, NUM_PAGES(size), KERNEL_MEM_START,
        KERNEL_MEM_END, MM_FLAGS_KERNEL_DATA);


    s = malloc(sizeof(*s));
    *s = (struct lio_userspace_service) {
        .provider   = current_thread,
        .ring       = (struct lio_ring) {
            .ring       = ring_init_writer(buf, size),
            .kern_base  = (uintptr_t) buf,
            .usp_base   = (uintptr_t) buffer,
            .pending    = tree_create(struct lio_pending_op, tinfo, op_addr),
        },
    };

    service->opaque = s;

    lio_add_service(service);

    services = current_thread->lio_services;
    if (!services) {
        services = current_thread->lio_services = list_create();
        pm_thread_register_on_destroy(current_thread,
                                      service_on_thread_destroy, NULL);
    }
    list_push(services, service);

    return 0;
}

/// Neuen Tree-Ring registrieren
int syscall_lio_srv_tree_set_ring(lio_usp_tree_t* ptree_id, tid_t tid,
    void* buffer, size_t size)
{
    lio_usp_tree_t tree_id;
    struct lio_tree* tree;
    struct lio_userspace_tree* t;
    pm_thread_t* thread;
    void* buf;

    // TODO is_userspace
    tree_id = *ptree_id;

    thread = pm_thread_get(current_process, tid);
    if (thread == NULL) {
        return -EINVAL;
    }

    tree = lio_usp_get_tree(tree_id);
    if (tree == NULL) {
        return -EBADF;
    }

    // TODO: Ein paar is_userspace
    if ((uintptr_t) buffer & (PAGE_SIZE - 1)) {
        return -EINVAL;
    }
    buf = mmc_automap_user(&mmc_current_context(),
        &mmc_current_context(), buffer, NUM_PAGES(size), KERNEL_MEM_START,
        KERNEL_MEM_END, MM_FLAGS_KERNEL_DATA);


    t = malloc(sizeof(*t));
    *t = (struct lio_userspace_tree) {
        .thread     = thread,
        .ring       = {
            .ring       = ring_init_writer(buf, size),
            .kern_base  = (uintptr_t) buf,
            .usp_base   = (uintptr_t) buffer,
            .pending    = tree_create(struct lio_pending_op, tinfo, op_addr),
        },
    };

    tree->opaque = t;

    if (!thread->lio_trees) {
        thread->lio_trees = list_create();
        pm_thread_register_on_destroy(thread, tree_on_thread_destroy, NULL);
    }
    list_push(thread->lio_trees, tree);

    return 0;
}

static inline bool is_power_of_two(size_t n)
{
    return !(n & (n - 1));
}

/// Ressource fuer Kernel bereitstellen
int syscall_lio_srv_res_upload(struct lio_server_resource* resource)
{
    struct lio_resource* res;
    bool translated = ((resource->flags & LIO_SRV_TRANSLATED) ? 1 : 0);

    // TODO: is_userspace
    if (resource->id) {
        res = lio_usp_get_resource(resource->id);
    } else {
        res = lio_create_resource();
        resource->id = res->usp_id;
    }

    if (!res) {
        return -1;
    }

    /* Die Blockgroesse muss ein Zweierpotenz sein */
    if (!is_power_of_two(resource->blocksize)) {
        return -EINVAL;
    }

    res->tree = lio_usp_get_tree(resource->tree);
    res->blocksize = resource->blocksize;
    res->readable = ((resource->flags & LIO_SRV_READABLE) ? 1 : 0);
    res->writable = ((resource->flags & LIO_SRV_WRITABLE) ? 1 : 0);
    res->seekable = ((resource->flags & LIO_SRV_SEEKABLE) ? 1 : 0);
    res->moredata = ((resource->flags & LIO_SRV_MOREDATA) ? 1 : 0);
    res->browsable = ((resource->flags & LIO_SRV_BROWSABLE) ? 1 : 0);
    res->changeable = ((resource->flags & LIO_SRV_CHANGEABLE) ? 1 : 0);
    res->resolvable = ((resource->flags & LIO_SRV_RESOLVABLE) ? 1 : 0);
    res->retargetable = ((resource->flags & LIO_SRV_RETARGETABLE) ? 1 : 0);
    res->ispipe = ((resource->flags & LIO_SRV_PIPE) ? 1 : 0);
    if (translated) {
        return -EINVAL;
    }
    res->is_terminal = ((resource->flags & LIO_SRV_IS_TERMINAL) ? 1 : 0);
    res->opaque = resource->opaque;

    /* Auch aufrufen falls sich die Größe nicht geändert hat: moredata könnte
     * von true auf false gewechselt sein. */
    lio_resource_update_size(res, resource->size);

    return 0;
}

/// Neuen Knoten erstellen
int syscall_lio_srv_node_add(lio_usp_resource_t* parent,
    lio_usp_resource_t* resource, const char* name, size_t name_len)
{
    struct lio_resource* par;
    struct lio_resource* res;

    // TODO: Ein paar is_userspace
    par = lio_usp_get_resource(*parent);
    res = lio_usp_get_resource(*resource);

    if (!par || !res) {
        return -1;
    }

    lio_create_node(par, res, name);

    return 0;
}

/// Knoten loeschen
int syscall_lio_srv_node_remove(lio_usp_resource_t* parent, const char* name,
    size_t name_len)
{
    struct lio_resource* par;
    struct lio_node* node;

    // TODO: Ein paar is_userspace
    par = lio_usp_get_resource(*parent);
    if (!par || !(node = lio_resource_get_child(par, name))) {
        return -1;
    }

    lio_resource_remove_child(par, node);
    return 0;
}

static void op_done(struct lio_op* op, tree_t* pending_ops,
                    int status, uint64_t result)
{
    struct lio_pending_op* pending;

    pending = tree_search(pending_ops, (uint64_t)(uintptr_t) op);
    if (pending != NULL) {
        *pending->status = status;
        if (pending->result) {
            *pending->result = result;
        }

        if (pending->caller->status == PM_STATUS_WAIT_FOR_IO_REQUEST) {
            pending->caller->status = PM_STATUS_READY;
        } else {
            panic("LIO-Request beendet fuer Thread, der gar nicht wartet");
        }

        tree_remove(pending_ops, pending);
        free(pending);
    }
}

/// Kernel ueber abgearbeitete Operation informieren
void syscall_lio_srv_op_done(struct lio_op* op, int status, uint64_t* result)
{

    struct lio_service* serv;
    struct lio_userspace_service* s;
    struct lio_tree* tree;
    struct lio_userspace_tree* t;
    int i;

    for (i = 0;
        (serv = list_get_element_at(current_thread->lio_services, i));
        i++)
    {
        s = serv->opaque;
        // TODO is_userspace
        op_done(op, s->ring.pending, status, *result);
    }

    for (i = 0;
        (tree = list_get_element_at(current_thread->lio_trees, i));
        i++)
    {
        t = tree->opaque;
        // TODO is_userspace
        op_done(op, t->ring.pending, status, *result);
    }
}

/**
 * Prueft ob der Prozess noch abzuarbeitende LIO-Operationen hat.
 *
 * @return 1 wenn ja, 0 sonst
 */
bool has_ops_todo(void)
{
    size_t i;
    struct lio_service* s;
    struct lio_userspace_service* us;
    struct lio_tree* t;
    struct lio_userspace_tree* ut;

    for (i = 0; (s = list_get_element_at(current_thread->lio_services, i)); i++) {
        us = s->opaque;
        if (!ring_empty(us->ring.ring)) {
            return true;
        }
    }

    for (i = 0; (t = list_get_element_at(current_thread->lio_trees, i)); i++) {
        ut = t->opaque;
        if (!ring_empty(ut->ring.ring)) {
            return true;
        }
    }

    return false;
}

/// Auf LIO-Operationen fuer diesen Prozess warten
void syscall_lio_srv_wait(void)
{
    if (has_ops_todo()) {
        return;
    }

    pm_scheduler_push(current_thread);
    current_thread->status = PM_STATUS_WAIT_FOR_LIO;
    current_thread = pm_scheduler_pop();
}

/**
 * Sucht ein Plaetzchen fuer eine Operation der angegebenen Groesse. Die
 * Operation wird als NOP mit der passenden Groesse und USED als Flags
 * initialisiert.
 */
static struct lio_op* alloc_ring_op(struct lio_ring* ring, size_t size)
{
    struct lio_op* op;
    void* buf;
    int ret;

    ret = ring_write_ptr(ring->ring, &buf, size);

    while (ret == -EBUSY) {
        pm_scheduler_yield();
        ret = ring_write_ptr(ring->ring, &buf, size);
    }

    op = buf;
    op->flags = LIO_OPFL_USED;

    return op;
}

static struct lio_op* alloc_op(struct lio_resource* res, size_t size)
{
    struct lio_tree* tree = res->tree;
    struct lio_userspace_tree* t = tree->opaque;
    struct lio_service* serv = tree->service;
    struct lio_userspace_service* s = serv->opaque;

    if (t == NULL || s->provider == NULL) {
        return NULL;
    }

    return alloc_ring_op(&t->ring, size);
}

/**
 * Entsperrt alle Threads die auf LIO-Operationen warten im angegebenen Prozess
 * und setzt den Ringzeiger weiter.
 */
static int commit_ring_op(struct lio_ring* ring, pm_thread_t* thread,
                          struct lio_op* op, uint64_t* result)
{
    struct lio_pending_op *pending;
    int ret, status;

    /* Operation in den Ring schreiben */
    ret = ring_write_complete(ring->ring);
    if (ret < 0) {
        panic("ring_write_complete fehlgeschlagen");
    }

    /* Service wieder laufen lassen, wenn er auf Requests wartet */
    if (thread->status == PM_STATUS_WAIT_FOR_LIO) {
        thread->status = PM_STATUS_READY;
    }

    /* Warten, bis der Request fertig ist */
    status = LIO_OPFL_USED;

    pending = malloc(sizeof(*pending));
    *pending = (struct lio_pending_op) {
        .caller     = current_thread,
        .op_addr    = (uintptr_t) op - ring->kern_base + ring->usp_base,
        .status     = &status,
        .result     = result,
    };
    tree_insert(ring->pending, pending);

    while (status == LIO_OPFL_USED) {
        pm_scheduler_kern_yield(thread->tid, PM_STATUS_WAIT_FOR_IO_REQUEST);
    }

    return status;
}

static int commit_op(struct lio_resource* res, struct lio_op* op,
    uint64_t* result)
{
    struct lio_tree* tree = res->tree;
    struct lio_userspace_tree* t = tree->opaque;
    struct lio_userspace_service* s = tree->service->opaque;

    if (s->provider == NULL) {
        return -EBADF;
    }

    if (t == NULL) {
        /* Noch nicht initialisiert */
        return -EBUSY;
    }

    return commit_ring_op(&t->ring, t->thread, op, result);
}

static struct lio_resource* load_root(struct lio_tree* tree)
{
    struct lio_resource* res = NULL;
    struct lio_op_load_root* olr;
    struct lio_service* serv = tree->service;
    struct lio_userspace_service* s = serv->opaque;
    uint64_t result;
    int ret;

    if (s->provider == NULL) {
        return NULL;
    }

    olr = (struct lio_op_load_root*) alloc_ring_op(&s->ring, sizeof(*olr));

    olr->op.opcode = LIO_OP_LOAD_ROOT;
    olr->tree = tree->usp_id;
    olr->op.flags |= LIO_OPFL_USP;

    /* FIXME Beim Unmount wieder freigeben */
    olr->source = -1;
    if (tree->source) {
        struct lio_usp_stream* fd =
            lio_usp_add_stream(s->provider->process, tree->source);
        if (fd) {
            olr->source = fd->usp_id;
        }
    }
    ret = commit_ring_op(&s->ring, s->provider, &olr->op, &result);
    if (ret == 0) {
        res = lio_usp_get_resource(result);
    }

    return res;
}

static int load_children(struct lio_resource* res)
{
    int result;
    struct lio_op_load_children* olc;

    olc = (struct lio_op_load_children*) alloc_op(res, sizeof(*olc));
    if (olc == NULL) {
        return -EBADF;
    }

    olc->op.opcode = LIO_OP_LOAD_CHILDREN;
    olc->resource = res->usp_id;
    olc->opaque = res->opaque;
    olc->op.flags |= LIO_OPFL_USP;

    result = commit_op(res, &olc->op, NULL);

    return result;
}

static void close(struct lio_stream* stream)
{
    struct lio_resource* res;
    struct lio_op_close* oc;

    BUG_ON(stream->is_composite);
    res = stream->simple.res;

    oc = (struct lio_op_close*) alloc_op(res, sizeof(*oc));
    if (oc == NULL) {
        return;
    }

    oc->op.opcode = LIO_OP_CLOSE;
    oc->resource = res->usp_id;
    oc->opaque = res->opaque;
    oc->op.flags |= LIO_OPFL_USP;

    commit_op(res, &oc->op, NULL);
}

static int read(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf)
{
    int result = 0;
    struct lio_op_read* or;
    struct lio_service* serv = res->tree->service;
    struct lio_userspace_service* s = serv->opaque;

    void* user_buf;
    void* buf_page = (void*) PAGE_ALIGN_ROUND_DOWN((uintptr_t) buf);
    size_t num_pages = NUM_PAGES(buf - buf_page + bytes);

    or = (struct lio_op_read*) alloc_op(res, sizeof(*or));
    if (or == NULL) {
        return -EBADF;
    }

    user_buf = mmc_automap_user(&s->provider->process->context,
        &mmc_current_context(), buf_page, num_pages,
        MM_USER_START, MM_USER_END, MM_FLAGS_USER_CODE);

    if (user_buf == NULL) {
        return -ENOMEM;
    }

    or->op.opcode = LIO_OP_READ;
    or->resource = res->usp_id;
    or->opaque = res->opaque;
    or->offset = offset;
    or->size = bytes;
    or->buf = user_buf + (buf - buf_page);
    or->op.flags |= LIO_OPFL_USP;

    result = commit_op(res, &or->op, NULL);

    mmc_unmap(&s->provider->process->context, user_buf, num_pages);

    return result;
}

static int write(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf)
{
    int result = 0;
    struct lio_op_write* ow;
    struct lio_service* serv = res->tree->service;
    struct lio_userspace_service* s = serv->opaque;

    void* user_buf;
    void* buf_page = (void*) PAGE_ALIGN_ROUND_DOWN((uintptr_t) buf);
    size_t num_pages = NUM_PAGES(buf - buf_page + bytes);

    ow = (struct lio_op_write*) alloc_op(res, sizeof(*ow));
    if (ow == NULL) {
        return -EBADF;
    }

    user_buf = mmc_automap_user(&s->provider->process->context,
        &mmc_current_context(), buf_page, num_pages,
        MM_USER_START, MM_USER_END, MM_FLAGS_USER_CODE);

    if (user_buf == NULL) {
        return -ENOMEM;
    }

    ow->op.opcode = LIO_OP_WRITE;
    ow->resource = res->usp_id;
    ow->res_size = res->size;
    ow->opaque = res->opaque;
    ow->offset = offset;
    ow->size = bytes;
    ow->buf = user_buf + (buf - buf_page);
    ow->op.flags |= LIO_OPFL_USP;

    result = commit_op(res, &ow->op, NULL);

    mmc_unmap(&s->provider->process->context, user_buf, num_pages);

    return result;
}

static int sync(struct lio_resource* res)
{
    int result = 0;
    struct lio_op_sync* os;

    os = (struct lio_op_sync*) alloc_op(res, sizeof(*os));
    if (os == NULL) {
        return -EBADF;
    }

    os->op.opcode = LIO_OP_SYNC;
    os->res = res->usp_id;
    os->opaque = res->opaque;
    os->op.flags |= LIO_OPFL_USP;

    result = commit_op(res, &os->op, NULL);

    return result;
}

static struct lio_resource* make_file(struct lio_resource* parent,
    const char* name)
{
    struct lio_resource* result;
    struct lio_op_make_file* omf;
    size_t name_len = strlen(name) + 1;
    uint64_t res_id;
    int ret;

    omf = (struct lio_op_make_file*) alloc_op(parent, sizeof(*omf) + name_len);
    if (omf == NULL) {
        return NULL;
    }

    omf->op.opcode = LIO_OP_MAKE_FILE;
    omf->parent = parent->usp_id;
    omf->opaque = parent->opaque;
    strcpy(omf->name, name);
    omf->op.flags |= LIO_OPFL_USP;

    ret = commit_op(parent, &omf->op, &res_id);

    if (ret < 0) {
        result = NULL;
    } else {
        result = lio_usp_get_resource(res_id);
    }

    return result;
}

static struct lio_resource* make_dir(struct lio_resource* parent,
    const char* name)
{
    struct lio_resource* result;
    struct lio_op_make_file* omd;
    size_t name_len = strlen(name) + 1;
    uint64_t res_id;
    int ret;

    omd = (struct lio_op_make_file*) alloc_op(parent, sizeof(*omd) + name_len);
    if (omd == NULL) {
        return NULL;
    }


    omd->op.opcode = LIO_OP_MAKE_DIR;
    omd->parent = parent->usp_id;
    omd->opaque = parent->opaque;
    strcpy(omd->name, name);
    omd->op.flags |= LIO_OPFL_USP;

    ret = commit_op(parent, &omd->op, &res_id);

    if (ret < 0) {
        result = NULL;
    } else {
        result = lio_usp_get_resource(res_id);
    }

    omd->op.flags &= ~LIO_OPFL_USED;

    return result;
}

static struct lio_resource* make_symlink(struct lio_resource* parent,
    const char* name, const char* target)
{
    struct lio_resource* result;
    struct lio_op_make_symlink* oms;
    size_t name_len = strlen(name) + 1;
    size_t target_len = strlen(target) + 1;
    uint64_t res_id;
    int ret;

    oms = (struct lio_op_make_symlink*)
        alloc_op(parent, sizeof(*oms) + name_len + target_len);
    if (oms == NULL) {
        return NULL;
    }

    oms->op.opcode = LIO_OP_MAKE_SYMLINK;
    oms->parent = parent->usp_id;
    oms->opaque = parent->opaque;
    oms->name_len = name_len;
    strcpy(oms->strings, name);
    strcpy(oms->strings + name_len, target);
    oms->op.flags |= LIO_OPFL_USP;

    ret = commit_op(parent, &oms->op, &res_id);

    if (ret < 0) {
        result = NULL;
    } else {
        result = lio_usp_get_resource(res_id);
    }

    return result;

}

static int truncate(struct lio_resource* res, uint64_t size)
{
    int result = 0;
    struct lio_op_truncate* otr;

    otr = (struct lio_op_truncate*) alloc_op(res, sizeof(*otr));
    if (otr == NULL) {
        return -EBADF;
    }

    otr->op.opcode = LIO_OP_TRUNCATE;
    otr->resource = res->usp_id;
    otr->opaque = res->opaque;
    otr->size = size;
    otr->op.flags |= LIO_OPFL_USP;

    result = commit_op(res, &otr->op, NULL);

    return result;
}

static int unlink(struct lio_resource* parent, const char* name)
{
    int result = 0;
    struct lio_op_unlink* oul;
    size_t name_len = strlen(name) + 1;

    oul = (struct lio_op_unlink*) alloc_op(parent, sizeof(*oul) + name_len);
    if (oul == NULL) {
        return -EBADF;
    }

    oul->op.opcode = LIO_OP_UNLINK;
    oul->parent = parent->usp_id;
    oul->opaque = parent->opaque;
    strcpy(oul->name, name);
    oul->op.flags |= LIO_OPFL_USP;

    result = commit_op(parent, &oul->op, NULL);

    return result;

}

static int probe(struct lio_service* serv, struct lio_stream* source,
                 struct lio_probe_service_result* probe_data)
{
    struct lio_userspace_service* s = serv->opaque;
    struct lio_probe_service_result* result;
    struct lio_usp_stream* usp_fd;
    struct lio_op_probe* op;
    void* user_buf;
    int ret;

    if (s->provider == NULL) {
        return -EBADF;
    }

    op = (struct lio_op_probe*) alloc_ring_op(&s->ring, sizeof(*op));

    usp_fd = lio_usp_add_stream(s->provider->process, source);
    if (!usp_fd) {
        return -EBADF;
    }

    BUILD_BUG_ON(sizeof(*result) > PAGE_SIZE);
    result = mmc_valloc(&mmc_current_context(), 1, MM_FLAGS_KERNEL_CODE);
    if (result == NULL) {
        ret = -ENOMEM;
        goto fail_alloc_page;
    }
    memset(result, 0, PAGE_SIZE);

    user_buf = mmc_automap_user(&s->provider->process->context,
                                &mmc_current_context(), result, 1,
                                MM_USER_START, MM_USER_END, MM_FLAGS_USER_DATA);
    if (user_buf == NULL) {
        ret = -ENOMEM;
        goto fail_map;
    }

    op->op.opcode = LIO_OP_PROBE;
    op->source = usp_fd->usp_id;
    op->probe_data = user_buf;

    ret = commit_ring_op(&s->ring, s->provider, &op->op, NULL);
    if (ret == 0) {
        strlcpy(result->service, serv->name, sizeof(result->service));
        memcpy(probe_data, result, sizeof(*probe_data));
    }

    mmc_unmap(&s->provider->process->context, user_buf, 1);
fail_map:
    mmc_vfree(&mmc_current_context(), result, 1);
fail_alloc_page:
    /* Wir dürfen kein close() machen, der Kernel braucht den Stream noch */
    source->usp_refcount++;
    lio_usp_unref_stream(s->provider->process, usp_fd);
    BUG_ON(--source->usp_refcount != 0);
    return ret;
}

static int pipe(struct lio_resource* res, struct lio_stream* pipe_end,
    int flags)
{
    int result = 0;
    struct lio_op_pipe* opi;
    struct lio_service* serv = res->tree->service;
    struct lio_userspace_service* s = serv->opaque;
    struct lio_usp_stream* fd;

    opi = (struct lio_op_pipe*) alloc_op(res, sizeof(*opi));
    if (opi == NULL) {
        return -EBADF;
    }

    fd = lio_usp_add_stream(s->provider->process, pipe_end);

    opi->op.opcode = LIO_OP_PIPE;
    opi->res = res->usp_id;
    opi->opaque = res->opaque;
    opi->pipe_end = fd->usp_id;
    opi->flags = flags;
    opi->op.flags |= LIO_OPFL_USP;

    result = commit_op(res, &opi->op, NULL);

    return result;
}
