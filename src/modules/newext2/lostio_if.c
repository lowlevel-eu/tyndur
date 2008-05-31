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

static bool handle_inode(uint64_t inode_num, char* name, vfstree_node_t* parent)
{
    ext2_inode_t* parent_inode = parent->data;
    ext2_inode_t* inode;

    inode = malloc(sizeof(ext2_inode_t));

    if (!ext2_inode_read(parent_inode->fs, inode_num, inode)) {
        return FALSE;
    }

    if (EXT2_INODE_IS_FILE(inode)) {
        vfstree_create_child(parent, name, EXT2_LOSTIO_TYPE_FILE,
            inode->raw.size, inode, 0);
    } else if (EXT2_INODE_IS_DIR(inode)) {
        vfstree_create_child(parent, name, EXT2_LOSTIO_TYPE_DIR, 0, inode,
            LOSTIO_FLAG_BROWSABLE);
        fs_fill_dir(vfstree_get_node_by_name(parent, name));
    } else {
        return FALSE;
    }

    return TRUE;
}

static int handle_dirent(ext2_dirent_t* dirent, void* prv)
{
    vfstree_node_t* node = prv;
    char* name;

    name = ext2_dir_alloc_name(dirent);

    if (strcmp(name, ".") && strcmp(name, "..")) {
        handle_inode(dirent->inode, name, node);
    }

    free(name);
    return 0;
}

static bool fs_fill_dir(vfstree_node_t* node)
{
    ext2_inode_t* inode= node->data;

    ext2_dir_foreach(inode, handle_dirent, node);
    return TRUE;
}

static bool fs_read_tree(ext2_fs_t* fs)
{
    char rp[ROOT_PATH_SIZE];
    vfstree_node_t* node;
    ext2_inode_t* root_inode = malloc(sizeof(*root_inode));

    if (!ext2_inode_read(fs, EXT2_ROOTDIR_INODE, root_inode)) {
        goto out_err;
    }

    get_root_path(fs->dev_private, rp);
    if ((!vfstree_create_node(rp, EXT2_LOSTIO_TYPE_DIR, 0, root_inode,
        LOSTIO_FLAG_BROWSABLE)) || (!(node = vfstree_get_node_by_path(rp))))
    {
        goto out_err;
    }

    if (!fs_fill_dir(node)) {
        goto out_err;
    }

    return TRUE;

out_err:
    free(root_inode);
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

static bool create_node(char* path, int type)
{
    ext2_inode_t* parent_inode;
    ext2_inode_t inode;
    int result;
    char* name = vfstree_basename(path);
    char* parent_path = vfstree_dirname(path);
    vfstree_node_t* parent_node = vfstree_get_node_by_path(parent_path);

    free(parent_path);

    if (!parent_node) {
        printf("parent_node not found '%s'\n", path);
        return FALSE;
    }

    parent_inode = parent_node->data;
    switch (type) {
        case 1:
            result = ext2_file_create(parent_inode, name, &inode);
            break;

        case 2:
            result = ext2_dir_create(parent_inode, name, &inode);
            break;
        
        case 3:
            puts("next2: Symlinks sind noch nicht implementiert");
        default:
            result = 0;
    }

    if (!result) {
        return FALSE;
    }

    return handle_inode(inode.number, name, parent_node);
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
        if ((args & IO_OPEN_MODE_CREATE) == 0) {
            goto out_err;
        }

        if (!create_node(*path, type)) {
            printf("next2: Fehler beim Anlegen des Knotens '%s'\n", *path);
            goto out_err;
        }
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

read_hdl_reply lostio_read_handler(lostio_filehandle_t* fh, size_t bs, size_t bc)
{
    static read_hdl_reply reply = {.data = NULL, .size = 0};
    size_t size;
    ext2_inode_t* inode = fh->node->data;

    p();

    size = bs * bc;
    if (size > fh->node->size - fh->pos) {
        size = fh->node->size - fh->pos;
    }
    if (!size) {
        reply.size = 0;
        goto out;
    }

    reply.size = size;
    reply.data = realloc(reply.data, reply.size);
    ext2_inode_readdata(inode, fh->pos, size, reply.data);

    fh->pos += size;
    if(fh->pos >= fh->node->size) {
        fh->flags |= LOSTIO_FLAG_EOF;
    } else {
        fh->flags &= ~LOSTIO_FLAG_EOF;
    }

out:
    v();
    return reply;
}

size_t lostio_write_handler(lostio_filehandle_t* fh, size_t bs, size_t bc,
    void* data)
{
    size_t size = bs * bc;
    ext2_inode_t* inode = fh->node->data;
    size_t result = 0;

    p();

    if (!size) {
        goto out;
    }

    if (!ext2_inode_writedata(inode, fh->pos, size, data)) {
        puts("next2: Fehler beim schreiben der Daten");
        goto out;
    }

    if (!ext2_inode_update(inode)) {
        puts("next2: Fehler beim schreiben des Inodes; Dadurch wurde "
            "vermutlich das Dateisystem inkonsistent!");
        goto out;
    }
    result = size;
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
            new_pos = fh->node->size;
            break;
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


