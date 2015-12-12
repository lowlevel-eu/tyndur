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

#include <stdint.h>
#include <stdbool.h>
#include <types.h>
#include <lostio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <errno.h>

#include "cdi/fs.h"

#define BLOCKSIZE 1024


static int ensure_loaded(struct cdi_fs_stream* s)
{
    if (s->res->loaded || s->res->res->load(s)) {
        return 1;
    }

    return 0;
}

static struct lio_resource* create_liores(struct cdi_fs_filesystem* fs,
    struct cdi_fs_res* cdires)
{
    struct lio_resource* liores;
    struct cdi_fs_stream s;

    s.res = cdires;
    s.fs = fs;
    s.error = 0;

    if (!ensure_loaded(&s)) {
        return NULL;
    }

    liores = lio_resource_create(fs->osdep.tree);
    liores->opaque = cdires;

    liores->moredata = 0;

    /*
     * CDI-Ressourcen können mehrere Klassen haben. Einige Kombinationen
     * sind semantischer Müll, z.B. Datei und Symlink gleichzeitig. Wir geben
     * Dateien den Vorzug. Gleichzeitig ein Verzeichnis zu sein ist vermutlich
     * auch keine gute Idee, aber theoretisch irgendwie denkbar.
     *
     * Außerdem prüfen wir noch, ob die Flags überhaupt zu den Klassen
     * passen.
     */
    if (cdires->file) {
        liores->readable = liores->seekable = cdires->flags.read;
        liores->writable = (cdires->flags.write && !fs->read_only);
    } else if (cdires->link) {
        liores->resolvable = cdires->flags.read_link;
        liores->retargetable = cdires->flags.write_link;
    }
    if (cdires->dir) {
        liores->browsable = cdires->flags.browse;
        liores->changeable = cdires->flags.create_child;
    }

    liores->size = cdires->res->meta_read(&s, CDI_FS_META_SIZE);
    liores->blocksize = BLOCKSIZE;

    lio_resource_modified(liores);

    return liores;
}

int cdi_fs_lio_probe(struct lio_service* service,
                     lio_stream_t source,
                     struct lio_probe_service_result* probe_data)
{
    struct cdi_fs_driver* driver = service->opaque;
    struct cdi_fs_filesystem* fs;
    char* volname = NULL;
    int ret;

    if (driver->fs_probe == NULL) {
        return -EINVAL;
    }

    fs = calloc(1, sizeof(*fs));
    fs->driver = driver;
    fs->osdep.source = source;

    ret = driver->fs_probe(fs, &volname);
    if (ret == 1) {
        strlcpy(probe_data->volname, volname, sizeof(probe_data->volname));
        ret = 0;
    } else {
        ret = -EINVAL;
    }

    free(volname);
    free(fs);

    return ret;
}

struct lio_resource* cdi_fs_lio_load_root(struct lio_tree* tree)
{
    struct cdi_fs_driver* driver = tree->service->opaque;
    struct cdi_fs_filesystem* fs;

    if (tree->source == -1ULL) {
        return NULL;
    }

    fs = calloc(1, sizeof(*fs));
    fs->driver = driver;
    fs->osdep.tree = tree;
    fs->osdep.source = tree->source;
    tree->opaque = fs;

    if (!driver->fs_init(fs)) {
        free(fs);
        return NULL;
    }

    return create_liores(fs, fs->root_res);
}

int cdi_fs_lio_load_children(struct lio_resource* res)
{
    struct cdi_fs_filesystem* fs = res->tree->opaque;
    struct cdi_fs_res* cdires = res->opaque;
    struct cdi_fs_stream s;
    cdi_list_t children;
    struct cdi_fs_res* child;
    struct lio_resource* liochild;
    size_t i;

    s.fs = fs;
    s.res = cdires;
    s.error = 0;

    if (!ensure_loaded(&s) || !cdires->dir) {
        return -1;
    }

    children = cdires->dir->list(&s);
    for (i = 0; (child = cdi_list_get(children, i)); i++) {
        liochild = create_liores(fs, child);
        if (!liochild) {
            return -1;
        }
        lio_node_create(res, liochild, child->name);
    }

    return 0;
}

int cdi_fs_lio_read_file(struct lio_resource* res, uint64_t offset,
    size_t bytes, void *buf)
{
    struct cdi_fs_filesystem* fs = res->tree->opaque;
    struct cdi_fs_res* cdires = res->opaque;
    struct cdi_fs_stream s;
    int result;

    s.fs = fs;
    s.res = cdires;
    s.error = 0;

    if (!ensure_loaded(&s) || !cdires->file || !cdires->file->read) {
        return -1;
    }

    if (offset + bytes > res->size) {
        /* TODO Das ist grosszuegig */
        memset(buf, 0, bytes);

        if (res->size > offset) {
            bytes = res->size - offset;
        } else {
            bytes = 0;
        }
    }

    if (bytes) {
        result = -!(cdires->file->read(&s, offset, bytes, buf) == bytes);
    } else {
        result = 0;
    }

    return result;
}

int cdi_fs_lio_read_symlink(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf)
{
    struct cdi_fs_filesystem* fs = res->tree->opaque;
    struct cdi_fs_res* cdires = res->opaque;
    struct cdi_fs_stream s;
    const char* str;
    size_t str_len;

    s.fs = fs;
    s.res = cdires;
    s.error = 0;

    if (!ensure_loaded(&s) || !cdires->link || !cdires->link->read_link) {
        return -1;
    }

    str = cdires->link->read_link(&s);
    str_len = strlen(str);

    if (offset + bytes > str_len) {
        bytes = str_len - offset;
    }

    memcpy(buf, str + offset, bytes);

    return 0;
}

int cdi_fs_lio_read(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf)
{
    if (res->resolvable) {
        return cdi_fs_lio_read_symlink(res, offset, bytes, buf);
    } else {
        return cdi_fs_lio_read_file(res, offset, bytes, buf);
    }
}

int cdi_fs_lio_write_file(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf)
{
    struct cdi_fs_filesystem* fs = res->tree->opaque;
    struct cdi_fs_res* cdires = res->opaque;
    struct cdi_fs_stream s;
    int result;

    s.fs = fs;
    s.res = cdires;
    s.error = 0;

    if (offset + bytes > res->size) {
        bytes = res->size - offset;
    }

    if (!ensure_loaded(&s) || !cdires->file || !cdires->file->write
        || fs->read_only)
    {
        return -1;
    }

    result = -!(cdires->file->write(&s, offset, bytes, buf) == bytes);

    return result;
}

int cdi_fs_lio_write_symlink(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf)
{
    struct cdi_fs_filesystem* fs = res->tree->opaque;
    struct cdi_fs_res* cdires = res->opaque;
    struct cdi_fs_stream s;
    int result;
    char* path;
    size_t path_len;

    s.fs = fs;
    s.res = cdires;
    s.error = 0;

    if (offset + bytes > res->size) {
        bytes = res->size - offset;
    }

    if (!ensure_loaded(&s) || !cdires->link || !cdires->link->write_link
        || fs->read_only)
    {
        return -1;
    }

    path = strdup(cdires->link->read_link(&s));
    path_len = strlen(path);
    if (path_len < offset + bytes) {
        path = realloc(path, offset + bytes + 1);
        path[offset + bytes] = '\0';
    }

    memcpy(path + offset, buf, bytes);
    result = -cdires->link->write_link(&s, path);

    return result;
}

int cdi_fs_lio_write(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf)
{
    if (res->retargetable) {
        return cdi_fs_lio_write_symlink(res, offset, bytes, buf);
    } else {
        return cdi_fs_lio_write_file(res, offset, bytes, buf);
    }
}

extern void caches_sync_all(void);
int cdi_fs_lio_sync(struct lio_resource* res)
{
    struct cdi_fs_filesystem* fs = res->tree->opaque;

    // FIXME Rückgabewert
    caches_sync_all();

    return lio_sync(fs->osdep.tree->source);
}

int cdi_fs_lio_resize(struct lio_resource* res, uint64_t size)
{
    struct cdi_fs_filesystem* fs = res->tree->opaque;
    struct cdi_fs_res* cdires = res->opaque;
    struct cdi_fs_stream s;

    s.fs = fs;
    s.res = cdires;
    s.error = 0;

    if (cdires->file) {
        if (!cdires->file->truncate || !cdires->file->truncate(&s, size)) {
            return -1;
        }
    } else if (!cdires->link) {
        return -1;
    }

    res->size = size;
    return 0;
}

static struct lio_resource* make_object(struct lio_resource* parent,
    const char* name, cdi_fs_res_class_t cls)
{
    struct cdi_fs_filesystem* fs = parent->tree->opaque;
    struct cdi_fs_res* cdiparent = parent->opaque;
    struct cdi_fs_stream s;
    struct lio_resource* res;

    s.fs = fs;
    s.res = NULL;
    s.error = 0;

    if (fs->read_only || !cdiparent->dir || !cdiparent->dir->create_child ||
        !cdiparent->dir->create_child(&s, name, cdiparent))
    {
        return NULL;
    }

    if (!ensure_loaded(&s)) {
        return NULL;
    }

    if (!s.res->res->assign_class(&s, cls)) {
        return NULL;
    }

    res = create_liores(fs, s.res);
    if (res) {
        lio_node_create(parent, res, name);
    }

    return res;
}

struct lio_resource* cdi_fs_lio_make_file(struct lio_resource* parent,
    const char* name)
{
    return make_object(parent, name, CDI_FS_CLASS_FILE);
}

struct lio_resource* cdi_fs_lio_make_dir(struct lio_resource* parent,
    const char* name)
{
    return make_object(parent, name, CDI_FS_CLASS_DIR);
}

struct lio_resource* cdi_fs_lio_make_symlink(struct lio_resource* parent,
    const char* name, const char* target)
{
    struct lio_resource* res;
    struct cdi_fs_res* cdires;
    struct cdi_fs_filesystem* fs = parent->tree->opaque;
    struct cdi_fs_stream s;

    res = make_object(parent, name, CDI_FS_CLASS_LINK);
    if (res) {
        cdires = res->opaque;
        s.fs = fs;
        s.res = cdires;
        s.error = 0;

        cdires->link->write_link(&s, target);

        res->size = strlen(target);
        lio_resource_modified(res);
    }

    return res;
}

/**
 * Alle Klassen von einer Ressource entfernen und sie dann loeschen.
 */
static int cdi_remove_res(struct cdi_fs_stream* stream)
{
    if (!stream->res->flags.remove)
    {
        return -1;
    }

    if (stream->res->file &&
        !stream->res->res->remove_class(stream,CDI_FS_CLASS_FILE))
    {
        return -1;
    }
    if (stream->res->dir &&
        !stream->res->res->remove_class(stream,CDI_FS_CLASS_DIR))
    {
        return -1;
    }
    if (stream->res->link &&
        !stream->res->res->remove_class(stream,CDI_FS_CLASS_LINK))
    {
        return -1;
    }

    if (!stream->res->res->remove(stream)) {
        return -1;
    }

    return 0;
}

int cdi_fs_lio_unlink(struct lio_resource* parent, const char* name)
{
    struct cdi_fs_filesystem* fs = parent->tree->opaque;
    struct cdi_fs_res* cdiparent = parent->opaque;
    struct cdi_fs_res* child;
    struct cdi_fs_stream s;
    struct lio_node* node;

    if (fs->read_only) {
        return -1;
    }

    s.fs = fs;
    s.res = cdiparent;
    s.error = 0;
    if (!ensure_loaded(&s)) {
        return -1;
    }

    if (!(node = lio_resource_get_child(parent, name))) {
        return -1;
    }
    child = node->res->opaque;

    s.res = child;
    if (!ensure_loaded(&s)) {
        return -1;
    }

    // Wir haben es mit einem Verzeichnis zu tun, das duerfen wir nur loeschen
    // wenn es keine Kindeintraege mehr hat.
    if (child->dir && cdi_list_size(child->children)) {
        return -1;
    }

    if (cdi_remove_res(&s)) {
        return -1;
    }
    lio_resource_remove_child(parent, node);

    return 0;
}

