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

#ifndef _INODE_H_
#define _INODE_H_

#include <stdint.h>

/// Inode
typedef struct ext2_inode_t {
    /// Dateimodus
    uint16_t mode;

    /// UID des Besitzers
    uint16_t uid;

    /// Groesse
    uint32_t size;

    /// Zeit des letzten Zugriffs
    uint32_t access_time;

    /// Erstellungszeit;
    uint32_t creation_time;

    /// Zeit der letzten Aenderungn
    uint32_t modification_time;

    /// Zeit der Loeschung
    uint32_t deletion_time;

    /// GID
    uint16_t gid;

    /// Anzahl der Links
    uint16_t link_count;

    /// Anzahl der Blocks.
    /// ACHTUNG: HIER HANDELT ES SICH UM 512-BYTE GROSSE BLOECKE!!!
    uint32_t block_count;

    /// Flags
    uint32_t flags;

    /// Reserviert
    uint32_t reserved1;

    /**
     *  Blocknummern. Fuer Blocks 0 bis 11 zeigt die Nummer direkt auf einen
     *  Datenblock der Datei, Block 12 ist indirekt, 13 doppelt indirekt und 14
     *  dreifach indirekt. Bei indirekter Adressierung zeigt die Blocknummer
     *  auf einen Block, der wiederum Blocknummern enthaelt.
     */
    uint32_t blocks[15];

    /// Version
    uint32_t version;

    /// TODO
    uint32_t file_acl;

    /// TODO
    uint32_t dir_acl;


    /// Fragment Adresse...
    uint32_t fragment_address;

    /// Fragment Nummer
    uint8_t fragment_number;

    /// Fragmentgroesse
    uint8_t fragment_size;

    uint16_t reserved2[5];
} __attribute__((packed)) ext2_inode_t;


// Makros fuer das Mode-Feld in Inodes
#define EXT2_INODE_MODE_FORMAT 0xF000
#define EXT2_INODE_MODE_SYMLINK 0xA000
#define EXT2_INODE_MODE_FILE 0x8000
#define EXT2_INODE_MODE_DIR 0x4000

#define EXT2_INODE_IS_FORMAT(i, f) (((i)->mode & EXT2_INODE_MODE_FORMAT) == f)
#define EXT2_INODE_IS_SYMLINK(i) EXT2_INODE_IS_FORMAT(i, \
    EXT2_INODE_MODE_SYMLINK)
#define EXT2_INODE_IS_FILE(i) EXT2_INODE_IS_FORMAT(i, EXT2_INODE_MODE_FILE)
#define EXT2_INODE_IS_DIR(i) EXT2_INODE_IS_FORMAT(i, EXT2_INODE_MODE_DIR)


int ext2_inode_read(ext2_fs_t* fs, uint64_t inode_nr, ext2_inode_t* inode);
int ext2_inode_readblk(ext2_fs_t* fs, ext2_inode_t* inode, uint64_t block,
    void* buf);
int ext2_inode_readdata(ext2_fs_t* fs, ext2_inode_t* inode, uint64_t start,
    size_t len, void* buf);

#endif
