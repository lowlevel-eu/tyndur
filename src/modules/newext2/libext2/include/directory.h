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

#ifndef _DIRECTORY_H_
#define _DIRECTORY_H_

#include <stdint.h>

/// Verzeichniseintrag wie er intern benutzt wird
typedef struct ext2_dirent {
    /// Inode-Nummer
    uint32_t inode;

    /// Laenge des aktuellen Eintrags
    uint16_t record_len;

    /// Laenge des Namens
    uint8_t name_len;

    /// Dateityp
    uint8_t type;

    /// Name (_NICHT_ nullterminiert)
    char name[];
} __attribute__((packed)) ext2_dirent_t;


/// Verzeichniseintraege verabeiten
void ext2_dir_foreach(ext2_fs_t* fs, ext2_inode_t* inode,
    int (*handler) (ext2_dirent_t*, void*), void* private);

/// Sucht einen Verzeichniseintrag nach dem Namen
ext2_dirent_t* ext2_dir_get(ext2_fs_t* fs, ext2_inode_t* inode,
    char* name);

/**
 * Nullterminierten String allozieren und mit dem Namen eines
 * Verzeichniseintrags fuellen
 */
char* ext2_dir_alloc_name(ext2_dirent_t* dirent);

#endif
