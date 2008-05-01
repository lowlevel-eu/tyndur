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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ext2.h"

/**
 * Prueft ob eine Inode-Nummer gueltig ist
 *
 * @param inode Inode-Nummer
 *
 * @return 1 wenn die Nummer gueltig ist, 0 sonst
 */
static inline int ext2_inode_valid(ext2_root_handle_t* root_handle,
    uint32_t inode)
{
    // Fuer ungueltige Inodenummern gibt es 2 Moeglichkeiten:
    // 0 oder groesser als die Anzahl der inodes
    if ((inode == 0) || (inode > root_handle->superblock.inode_count)) {
        return 0;
    }

    return 1;
}

/**
 * Gruppennummer in der ein Inode liegt ausfindig machen
 *
 * @param inode Inodenummer
 *
 * @return Gruppennummer
 */
static inline uint32_t ext2_inode_group(ext2_root_handle_t* root_handle,
    uint32_t inode)
{
    // Bei den Inodes wird bei 1 mit Zaehlen begonnen, das passt uns aber nicht
    // wirklich.
    inode--;

    // Die Gruppennummer ergibt sich daraus, dass die Inode-Nummer durch die
    // Anzahl der Inodes Pro Gruppe geteilt wird
    return inode / root_handle->superblock.inodes_per_group;
}

/**
 * Offset eines Inodes vom Anfang der Inode-Tabelle in Inodes
 *
 * @param inode Inode-Nummer
 *
 * @return Offset
 */
static inline uint32_t ext2_inode_group_offset(ext2_root_handle_t* root_handle,
    uint32_t inode)
{
    // Auch hier wieder die Geschichte mit dem Zaehlen ab 1
    inode--;

    // Dieser Offset ergibt sich, wenn die Gruppennummer Modulo Anzahl der
    // Inodes pro Gruppe genommen wird
    return inode % root_handle->superblock.inodes_per_group;
}

/**
 * Offset eines Inodes innerhalb des Blockes (in inodes)
 *
 * @param Inode-Nummer
 *
 * @return Offset
 */
static inline uint32_t ext2_inode_block_offset(ext2_root_handle_t* root_handle,
    uint32_t inode)
{
    // Inodes pro Block
    uint32_t inodes_per_block = ext2_block_size(root_handle) / 
        sizeof(ext2_inode_t);

    return ext2_inode_group_offset(root_handle, inode) % inodes_per_block;
}

/**
 * Blocknummer in der ein Inode zu finden ist
 *
 * @param inode Inode-Nummer
 *
 * @return Blocknummer
 */
static inline uint32_t ext2_inode_block(ext2_root_handle_t* root_handle,
    uint32_t inode)
{
    uint32_t group;
    uint32_t offset;
    uint32_t block;

     // Zuerst wird Ausgerechnet, in welcher Gruppe der Inode liegt
    group = ext2_inode_group(root_handle, inode);
    
    // Danach der Offset(in Inodes) vom Anfang der Inode_table
    offset = ext2_inode_group_offset(root_handle, inode);

    // Die Blocknummer...
    block = root_handle->group_descs[group].inode_table;
    //printf("inode_block group=%u  offset=%u  block=%u\n", group, offset, block);
    block += offset * sizeof(ext2_inode_t) / ext2_block_size(root_handle);

    return block;
}

/**
 * Inode anhand der Nummer einlesen
 *
 * @param handle Inode-Handle; Nur die Nummer muss gesetzt sein, die Daten
 *        werden in .data abgelegt.
 *
 * @return 0 Bei Erfolg, im Fehlerfall einer der Fehlercodes:
 *      -1: Ungueltige Inode-Nummer
 *      -2: Block der das Inode enthalten sollte, konnte nicht geladen werden
 */
int ext2_inode_read(ext2_root_handle_t* root_handle, ext2_ih_t* handle)
{
    uint32_t block;
    ext2_inode_t* inode_buf;
    uint8_t byte_buffer[ext2_block_size(root_handle)];

    // Bei einer ungueltigen Inode-Nummer wird sofort abgebrochen
    if (ext2_inode_valid(root_handle, handle->number) == 0) {
        return -1;
    }
    
    block = ext2_inode_block(root_handle, handle->number);

    // Jetzt sind die noetigen Daten bereit, und der entsprechende Block wird
    // gelesen
    if (ext2_block_read(root_handle, block, 1, byte_buffer) != 0) {
        return -2;
    }

    // Inode kopieren
    inode_buf = (ext2_inode_t*) byte_buffer;
    *handle->data = inode_buf[ext2_inode_block_offset(root_handle, handle->
        number)];

    return 0;
}   

/**
 * Inode speichern
 *
 * @param handle Inode-Handle
 *
 * @return 0 bei Erfolg, im Fehlerfall einer der folgenden Ferhlecodes:
 *      -1: Undgueltige Inode-Nummer
 *      -2: Fehler beim Einlesen
 *      -3: Fehler beim Schreiben
 *      -4: Inode-Daten nicht geladen
 */
int ext2_inode_write(ext2_root_handle_t* root_handle, ext2_ih_t* handle)
{
    uint32_t block;
    ext2_inode_t* inode_buf;
    uint8_t byte_buffer[ext2_block_size(root_handle)];

    // Bei einer ungueltigen Inode-Nummer wird sofort abgebrochen
    if (ext2_inode_valid(root_handle, handle->number) == 0) {
        return -1;
    }
    
    block = ext2_inode_block(root_handle, handle->number);
    inode_buf = (ext2_inode_t*) byte_buffer;

    // Zuerst einlesen, da sonst die anderen Inodes im selben Block
    // ueberschrieben werden
    if (ext2_block_read(root_handle, block, 1, byte_buffer) != 0) {
        return -2;
    }

    // Inode ueberschreiben
    inode_buf[ext2_inode_block_offset(root_handle, handle->number)] = *handle->
        data;

    // Block wieder schreiben
    if (ext2_block_write(root_handle, block, 1, &byte_buffer) != 0) {
        return -3;
    }
    handle->modified = 0;

    return 0;
}

