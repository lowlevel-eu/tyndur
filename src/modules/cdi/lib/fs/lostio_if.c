/*
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
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

#include <lostio.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

#include "cdi/fs.h"

// FIXME -- doppelt mit fs.c
#define CDI_TYPEHANDLE 200
#define CDI_TYPEHANDLE_SYMLINK 201

#define debug_print(x) puts(x)

// So lang kann der Root-Pfad maximal werden
#define ROOT_PATH_SIZE 27
#define FULL_PATH_SIZE(x) (strlen(x) + ROOT_PATH_SIZE)

static bool fs_fill_dir(vfstree_node_t* node);


extern struct cdi_fs_driver* the_one_and_only_driver;

/**
 * Diese Funktion generiert den Root-Pfad zu einem Source-Handle
 *
 * @param source Source-Handle
 * @param buffer Puffer, in dem der Pfad abgelegt werden soll (muss mindestens
 *               MAX_ROOT_PATH_SIZE Bytes lang sein.
 */
static inline void get_root_path(io_resource_t* source, char* buffer)
{
    snprintf(buffer, ROOT_PATH_SIZE, "/%08d_%016d", source->pid, (dword)
        source->resid);
}

/**
 * Diese Funktion generiert den vollen Pfad zu einem Source-Handle
 *
 * @param source Source-Handle
 * @param path Pfad
 * @param buffer Puffer, in dem der Pfad abgelegt werden soll (muss mindestens
 *               FULL_PATH_SIZE Bytes lang sein.
 */
static inline void get_full_path(io_resource_t* source, char* path,
    char* buffer)
{
    size_t size = FULL_PATH_SIZE(path);
    // Abschliessender Slash rauswerfen
    if (path[strlen(path) - 1] == '/') {
        size--;
    }

    snprintf(buffer, size, "/%08d_%016d%s", source->pid,
        (dword) source->resid, path);
}

static int handle_dirent(struct cdi_fs_res* res, vfstree_node_t* node)
{
    char* name;
    struct cdi_fs_stream* parent_stream = node->data;
    struct cdi_fs_stream* s;
    int flags;
    int type;
    uint64_t size;

    name = res->name;

    if (!strcmp(name, ".") || !strcmp(name, "..")) {
        return 0;
    }

    s = malloc(sizeof(*s));
    s->fs = parent_stream->fs;
    s->res = res;


    if (!res->loaded) {
        res->res->load(s);
    }

    flags = 0;
    size = 0;
    type = CDI_TYPEHANDLE;
    if (res->dir) {
        flags |= LOSTIO_FLAG_BROWSABLE;
        type = LOSTIO_TYPES_DIRECTORY;
    } else if (res->link) {
        flags |= LOSTIO_FLAG_SYMLINK;
        type = CDI_TYPEHANDLE_SYMLINK;
    } else {
        type = CDI_TYPEHANDLE;

        if (res->file) {
            size = res->res->meta_read(s, CDI_FS_META_SIZE);
        }
    }


    vfstree_create_child(node, name, type, size, s, flags);

    if (res->dir) {
        fs_fill_dir(vfstree_get_node_by_name(node, name));
    }

    return 0;
}

static bool fs_fill_dir(vfstree_node_t* node)
{
    struct cdi_fs_stream* stream = node->data;

    cdi_list_t children;
    struct cdi_fs_res* cres;
    int i;

    if (!stream->res->loaded) {
        stream->res->res->load(stream);
    }

    if (stream->res->dir) {
        children = stream->res->dir->list(stream);

        for (i = 0; (cres = cdi_list_get(children, i)); i++) {
            handle_dirent(cres, node);
        }
    }

    return TRUE;
}

static bool fs_read_tree(struct cdi_fs_filesystem* fs)
{
    char rp[ROOT_PATH_SIZE];
    vfstree_node_t* node;
    struct cdi_fs_stream* dh;

    // Dirhandle = Verknuepfung zwischen LIO und Treiber
    // FIXME Dahinter kommen eigentlich noch private Daten des Treibers...
    dh = malloc(sizeof(*dh));
    dh->fs = fs;
    dh->res = fs->root_res;

    get_root_path(fs->device, rp);
    if ((!vfstree_create_node(rp, LOSTIO_TYPES_DIRECTORY, 0, dh,
        LOSTIO_FLAG_BROWSABLE)) || (!(node = vfstree_get_node_by_path(rp))))
    {
        goto out_err;
    }

    if (!fs_fill_dir(node)) {
        goto out_err;
    }

    return TRUE;

out_err:
    free(dh);
    return FALSE;
}

static bool fs_mount(io_resource_t* source)
{
    struct cdi_fs_filesystem* fs = malloc(sizeof(*fs));
    struct cdi_fs_driver* drv = the_one_and_only_driver;

    // FIXME ;-)
    fs->device = malloc(sizeof(*source));
    *((io_resource_t*) fs->device) = *source;

    if (!drv->fs_init(fs)) {
        goto out_err;
    }

    if (!fs_read_tree(fs)) {
        goto out_err;
    }

    return TRUE;

out_err:
    free(fs->device);
    free(fs);
    return FALSE;
}

/**
 * Dieser Handler wird bei jedem fopen() aufgerufen, da das immer
 * schief gehen wird weil ja die daten nicht direkt auf Root liegen. Dabei
 * werden nur die Pfade richtig umgebeogen und es wird geprueft ob alle
 * notwendigen Verzeichnisse geladen sind
 */
bool lostio_not_found_handler(char** path, byte args, pid_t pid,
    io_resource_t* source)
{
    char root_path[ROOT_PATH_SIZE + 1];
    char full_path[FULL_PATH_SIZE(*path) + 1];
    vfstree_node_t* root_node;
    cdi_fs_res_class_t type;


    p();
    get_root_path(source, root_path);
    get_full_path(source, *path, full_path);

    // Wenn der Knoten nicht existiert muss gemountet werden
    root_node = vfstree_get_node_by_path(root_path);

    if (!root_node) {
        if (!fs_mount(source)) {
            // Wenn das mounten nicht klappt wird abgebrochen
            puts("Fehler beim Mount!");
            goto out_err;
        }

        root_node = vfstree_get_node_by_path(root_path);
    }

    // Testen ob ein Verzeichnis oder eine Datei geoeffnet werden soll
    if ((args & IO_OPEN_MODE_DIRECTORY) != 0) {
        type = CDI_FS_CLASS_DIR;
    } else if ((args & IO_OPEN_MODE_LINK) != 0) {
        type = CDI_FS_CLASS_LINK;
    } else {
        type = CDI_FS_CLASS_FILE;
    }

    // Pfad aendern
    *path = malloc(strlen(full_path) + 1);
    memcpy(*path, full_path, strlen(full_path) + 1);

    // Jetzt wird der Knoten geholt, damit geprueft werden kann, ob auch kein
    // Verzeichnis als Datei geoeffnet werden soll und sowas ;-)
    vfstree_node_t* node = vfstree_get_node_by_path(*path);
    struct cdi_fs_stream* stream;

    // Abbrechen falls der Knoten nicht existiert und auch nicht erstellt
    // werden soll
    if ((node == NULL) && ((args & IO_OPEN_MODE_CREATE) == 0)) {
        goto out_err;
    } else if ((node == NULL)) {
        // Der Knoten exisitiert nicht, soll aber erstellt werden
        char* parent_path = vfstree_dirname(*path);
        char* child_name = vfstree_basename(*path);
        vfstree_node_t* parent = vfstree_get_node_by_path(parent_path);
        free(parent_path);

        if (parent == NULL) {
            goto out_err;
        }

        // Kindeintraege koennen nur in Verzeichnissen angelegt werden
        struct cdi_fs_stream* parent_stream = parent->data;
        if (parent_stream->res->dir == NULL) {
            goto out_err;
        }

        // Stream erstellen fuer die neue Ressource
        struct cdi_fs_stream stream;;
        memset(&stream, 0, sizeof(stream));
        stream.fs = parent_stream->fs;

        if (!parent_stream->res->dir->create_child(&stream, child_name,
            parent_stream->res))
        {
            puts("cdi_fs: Fehler beim Anlegen der Kindressource");
            goto out_err;
        }

        if (!stream.res->res->assign_class(&stream, type)) {
            puts("cdi_fs: Fehler beim Zuweisen der Klasse fuer die neue"
                "Ressource");
            goto out_err;
        }

        handle_dirent(stream.res, parent);
        node = vfstree_get_node_by_name(parent, child_name);
    }

    stream = node->data;

    if (stream->res->link) {
        type = CDI_FS_CLASS_LINK;
    }

    switch (type) {
        case CDI_FS_CLASS_FILE:
            if (stream->res->file == NULL) {
                goto out_err;
            }

            if ((args & IO_OPEN_MODE_TRUNC)) {
                if (!stream->res->file->truncate(stream, 0)) {
                    puts("cdi_fs: Truncate fehlgeschlagen!");
                    goto out_err;
                }
                node->size = 0;
            }
            break;

        case CDI_FS_CLASS_DIR:
            if (stream->res->dir == NULL) {
                goto out_err;
            }
            break;

        case CDI_FS_CLASS_LINK:
            if (stream->res->link == NULL) {
                goto out_err;
            }
            break;

        default:
            goto out_err;
    }

    v();
    return TRUE;

out_err:
    v();
    return FALSE;
}

/**
 * Handler der dazu benutzt wird, die Pfade umzubiegen
 */
bool lostio_pre_open_handler(char** path, byte args, pid_t pid,
    io_resource_t* source)
{
    if ((strcmp(*path, "/") == 0) || (vfstree_get_node_by_path(*path) == NULL))
    {
        if (lostio_not_found_handler(path, args, pid, source) == FALSE) {
            return FALSE;
        }
    }
    return TRUE;
}

size_t lostio_read_handler(lostio_filehandle_t* fh, void* buf,
    size_t bs, size_t bc)
{
    size_t size = 0;
    struct cdi_fs_stream* dh = fh->node->data;

    p();

    if (dh->res->file == NULL) {
        goto out;
    }

    size = bs * bc;
    if (size > fh->node->size - fh->pos) {
        size = fh->node->size - fh->pos;
    }
    if (size == 0) {
        goto out;
    }

    size = dh->res->file->read(dh, fh->pos, size, buf);

    fh->pos += size;
    if(fh->pos >= fh->node->size) {
        fh->flags |= LOSTIO_FLAG_EOF;
    } else {
        fh->flags &= ~LOSTIO_FLAG_EOF;
    }

out:
    v();
    return size;
}

size_t lostio_write_handler(lostio_filehandle_t* fh, size_t bs, size_t bc,
    void* data)
{
    size_t size = bs * bc;
    struct cdi_fs_stream* stream = fh->node->data;
    size_t result = 0;

    p();

    if (!size) {
        goto out;
    }

    result = stream->res->file->write(stream, fh->pos, size, data);
    fh->pos += size;

    if (fh->pos > fh->node->size) {
        fh->node->size = fh->pos;
    }

    if(fh->pos >= fh->node->size) {
        fh->flags |= LOSTIO_FLAG_EOF;
    } else {
        fh->flags &= ~LOSTIO_FLAG_EOF;
    }

out:
    v();
    return result;
}



int lostio_seek_handler(lostio_filehandle_t* fh, uint64_t offset, int origin)
{
    uint64_t new_pos;

    p();

    switch(origin)
    {
        case SEEK_SET:
            new_pos = offset;
            break;

        case SEEK_CUR:
            new_pos = fh->pos + offset;
            break;

        case SEEK_END:
            new_pos = fh->node->size + offset;
            break;

        default:
            return -1;
    }

    if ((new_pos > fh->node->size) && ((origin == SEEK_SET) ||
        (origin == SEEK_CUR)) && ((fh->flags & IO_OPEN_MODE_WRITE) == 0))
    {
        v();
        return -1;
    }

    fh->pos = new_pos;

    if(fh->pos >= fh->node->size) {
        fh->flags |= LOSTIO_FLAG_EOF;
    } else {
        fh->flags &= ~LOSTIO_FLAG_EOF;
    }

    v();
    return 0;
}

/**
 * Alle Klassen von einer Ressource entfernen und sie dann loeschen.
 */
static int cdi_remove_res(struct cdi_fs_stream* stream)
{
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

int lostio_unlink_handler(lostio_filehandle_t* fh, const char* name)
{
    struct cdi_fs_stream* stream;
    vfstree_node_t* node;
    int i;
    int result = 0;

    p();

    // Passender Kindknoten finden
    for (i = 0; (node = list_get_element_at(fh->node->children, i)); i++) {
        if (!strcmp(node->name, name)) {
            goto found;
        }
    }

    result = -1;
    goto out;

found:
    stream = node->data;

    // Spezialfall: Es handelt sich um ein Verzeichnis. Das heisst es darf
    // nur geloescht werden wenn es keine Kind-Eintraege mehr hat.
    if (stream->res->dir && list_size(node->children)) {
        result = -1;
        goto out;
    }

    // Sonst kann die Ressource geloescht werden.
    if (cdi_remove_res(stream) || !vfstree_delete_child(fh->node, name)) {
        result = -1;
    }

out:
    v();
    return result;
}

size_t lostio_symlink_read_handler(lostio_filehandle_t* fh, void* buf,
    size_t bs, size_t bc)
{
    size_t size = bs*bc;
    struct cdi_fs_stream* dh = fh->node->data;
    const char* link;

    p();

    if (dh->res->link == NULL) {
        goto out;
    }

    link = dh->res->link->read_link(dh);
    if (size > strlen(link) + 1) {
        size = strlen(link) + 1;
    }
    strncpy(buf, link, size);

out:
    v();
    return size;
}

size_t lostio_symlink_write_handler(lostio_filehandle_t* fh, size_t bs,
    size_t bc, void* data)
{
    size_t size = bs * bc;
    struct cdi_fs_stream* stream = fh->node->data;
    size_t result = 0;

    p();

    if ((stream->res->link == NULL) || (((char*) data)[size - 1] != 0)) {
        goto out;
    }

    if (!size) {
        goto out;
    }

    if (stream->res->link->write_link(stream, data)) {
        result = size;
    }

out:
    v();
    return result;
}

