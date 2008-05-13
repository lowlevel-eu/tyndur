/*
 * Copyright (c) 2008 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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

#include <string.h>
#include "ext2.h"

struct private_dir_get {
    char* name;
    size_t name_len;
    ext2_dirent_t* result;
    int found;
};

void ext2_dir_foreach(ext2_fs_t* fs, ext2_inode_t* inode,
    int (*handler) (ext2_dirent_t*, void*), void* private)
{
    ext2_dirent_t* entry;
    char buf[inode->raw.size];
    uint64_t pos = 0;

    ext2_inode_readdata(inode, 0, inode->raw.size, buf);
    while (pos < inode->raw.size) {
        entry = (ext2_dirent_t*) &buf[pos];

        if (handler(entry, private)) {
            break;
        }

        if (entry->record_len == 0) {
            // TODO: Was geschieht mit dem kaputten FS?
            break;
        }
        pos += entry->record_len;
    }
}

static int dir_get_handler(ext2_dirent_t* dirent, void* prv)
{
    struct private_dir_get* p = prv;

    if (p->name_len != dirent->name_len) {
        return 0;
    }

    if (!strncmp(p->name, dirent->name, p->name_len)) {
        p->result = malloc(dirent->record_len);
        memcpy(p->result, dirent, dirent->record_len);
        return 1;
    }

    return 0;
}

ext2_dirent_t* ext2_dir_get(ext2_fs_t* fs, ext2_inode_t* inode,
    char* name)
{
    struct private_dir_get prv = {
        .name = name,
        .name_len = strlen(name),
        .result = NULL
    };

    ext2_dir_foreach(fs, inode, dir_get_handler, &prv);
    return prv.result;
}

char* ext2_dir_alloc_name(ext2_dirent_t* dirent)
{
    char* buf = malloc(dirent->name_len + 1);
    memcpy(buf, dirent->name, dirent->name_len);
    buf[dirent->name_len] = '\0';
    return buf;
}
