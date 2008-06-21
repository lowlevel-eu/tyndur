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

#define debug_print(x) puts(x)

// So lang kann der Root-Pfad maximal werden
#define ROOT_PATH_SIZE 27
#define FULL_PATH_SIZE(x) (strlen(x) + ROOT_PATH_SIZE)

static bool fs_fill_dir(vfstree_node_t* node);

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

    name = res->name;

    if (!strcmp(name, ".") || !strcmp(name, "..")) {
        return 0;
    }

    s = malloc(sizeof(*s));
    s->fs = parent_stream->fs;

    flags = 0;
    if (res->dir) {
        flags |= LOSTIO_FLAG_BROWSABLE;
    }

    vfstree_create_child(node, name, CDI_TYPEHANDLE,
        0 /* FIXME (?) */, res, flags);

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
    dh = malloc(sizeof(*dh));
    dh->fs = fs;
    dh->res = NULL; /* FIXME Woher bekommt man das Rootverzeichnis? */

    get_root_path(fs->device, rp);
    if ((!vfstree_create_node(rp, CDI_TYPEHANDLE, 0, dh,
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
    struct cdi_fs_driver* drv = NULL; /* FIXME Woher? */

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
    int type;

    p();

    get_root_path(source, root_path);
    get_full_path(source, *path, full_path);

    // Wenn der Knoten nicht existiert muss gemountet werden
    root_node = vfstree_get_node_by_path(root_path);

    if (!root_node) {
        if (!fs_mount(source)) {
            // Wenn das mounten nicht klappt wird abgebrochen
            printf("Fehler beim Mount!");
            goto out_err;
        }

        root_node = vfstree_get_node_by_path(root_path);
    }

    // Testen ob ein Verzeichnis oder eine Datei geoeffnet werden soll
    // FIXME Konstanten statt magischen Zahlen
    type = 1;
    if ((args & IO_OPEN_MODE_DIRECTORY) != 0) {
        type = 2;
    } else if ((args & IO_OPEN_MODE_LINK) != 0) {
        type = 3;
    }

    // Pfad aendern
    *path = malloc(strlen(full_path) + 1);
    memcpy(*path, full_path, strlen(full_path) + 1);

    // Jetzt wird der Knoten geholt, damit geprueft werden kann, ob auch kein
    // Verzeichnis als Datei geoeffnet werden soll und sowas ;-)
    vfstree_node_t* node = vfstree_get_node_by_path(*path);
    struct cdi_fs_stream* stream;

    if (node == NULL) {
        goto out_err;
    }

    stream = node->data;
    switch (type) {
        case 1:
            if (stream->res->file == NULL) {
                goto out_err;
            }
            break;
        case 2:
            if (stream->res->dir == NULL) {
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

read_hdl_reply lostio_read_handler(lostio_filehandle_t* fh, size_t bs, size_t bc)
{
    static read_hdl_reply reply = {.data = NULL, .size = 0};
    size_t size;
    struct cdi_fs_stream* dh = fh->node->data;

    if (dh->res->file == NULL) {
        goto out;
    }

    p();

    size = bs * bc;
    if (size > fh->node->size - fh->pos) {
        size = fh->node->size - fh->pos;
    }
    if (size == 0) {
        reply.size = 0;
        goto out;
    }

    reply.data = realloc(reply.data, size);
    reply.size = dh->res->file->read(dh, fh->pos, size, reply.data);

    fh->pos += reply.size;
    if(fh->pos >= fh->node->size) {
        fh->flags |= LOSTIO_FLAG_EOF;
    } else {
        fh->flags &= ~LOSTIO_FLAG_EOF;
    }

out:
    v();
    return reply;
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
            new_pos = fh->node->size;
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

