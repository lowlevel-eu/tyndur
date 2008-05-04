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

#include "lostio_if.h"
#include <ext2.h>

#define debug_print(x) puts(x)

// So lang kann der Root-Pfad maximal werden
#define ROOT_PATH_SIZE 27
#define FULL_PATH_SIZE(x) (strlen(x) + ROOT_PATH_SIZE)

/// Verzeichnis-Handle
typedef struct {
    /// Inode-Nummer
    uint32_t inode_num;

    /// Root-Handle
    ext2_fs_t* fs;
} dir_handle_t;

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

static int handle_dirent(ext2_dirent_t* dirent, void* prv)
{
    vfstree_node_t* node = prv;
    char* name;
    dir_handle_t* parent_dh;
    dir_handle_t* dh;

    name = ext2_dir_alloc_name(dirent);

    parent_dh = node->data;

    dh = malloc(sizeof(*dh));
    dh->fs = parent_dh->fs;
    dh->inode_num = dirent->inode;

    vfstree_create_child(node, name, EXT2_LOSTIO_TYPE_FILE, 0, dh, 0);

    free(name);
    return 0;
}

static bool fs_fill_dir(vfstree_node_t* node)
{
    dir_handle_t* dh = node->data;
    ext2_fs_t* fs = dh->fs;
    ext2_inode_t inode;

    if (!ext2_inode_read(fs, dh->inode_num - 1, &inode)) {
        return FALSE;
    }

    ext2_dir_foreach(fs, &inode, handle_dirent, node);
    return TRUE;
}

static bool fs_read_tree(ext2_fs_t* fs)
{
    char rp[ROOT_PATH_SIZE];
    vfstree_node_t* node;
    dir_handle_t* dh;

    // Dirhandle = Verknuepfung zwischen LIO und libext2
    dh = malloc(sizeof(*dh));
    dh->fs = fs;
    dh->inode_num = 2; // Makro?

    get_root_path(fs->dev_private, rp);
    if ((!vfstree_create_node(rp, EXT2_LOSTIO_TYPE_DIR, 0, dh,
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
    ext2_fs_t* fs = malloc(sizeof(*fs));

    // FIXME ;-)
    fs->dev_private = malloc(sizeof(*source));
    *((io_resource_t*) fs->dev_private) = *source;

    fs->dev_read = dev_read;
    fs->dev_write = dev_write;

    if (!ext2_fs_mount(fs)) {
        goto out_err;
    }

    if (!fs_read_tree(fs)) {
        goto out_err;
    }

    return TRUE;

out_err:
    free(fs->dev_private);
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
    if (node == NULL) {
        goto out_err;
    } else if ((node->type == EXT2_LOSTIO_TYPE_DIR) && (type != 2)) {
        goto out_err;
    } else if ((node->type == EXT2_LOSTIO_TYPE_FILE) && (type != 1)) {
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

