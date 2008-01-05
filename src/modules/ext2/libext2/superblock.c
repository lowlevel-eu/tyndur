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

#include "ext2.h"

/**
 * Superblock einlesen
 *
 * @return 0 Bei Erfolg, im Fehlerfall einer der Fehlercodes:
 *      -1: Fehler beim Seek
 *      -2: Fehler beim Lesen
 */
int ext2_superblock_read(ext2_root_handle_t* root_handle)
{
    // Hier wird im Moment einfach mal fix der erste genommen
    root_handle->superblock_block = 1;
    uint32_t block_size = 1024;

    // Superblock suchen
    if (fseek(root_handle->device, root_handle->superblock_block * block_size,
        SEEK_SET) != 0)
    {
        return -1;
    }

    // Superblock einlesen
    if (fread(&root_handle->superblock, 1, sizeof(root_handle->superblock),
        root_handle->device) != sizeof(root_handle->superblock))
    {
        return -2;
    }
    
    if (ext2_block_size(root_handle) > 1024) {
        root_handle->superblock_block = 0;
    }

    return 0;
}

/**
 * Superblock an eine bestimmte Stelle speichern
 *
 * @param Offset
 *
 * @return 0 bei Erfolg, im Fehlerfall < 0
 */
static int ext2_superblock_write_to(ext2_root_handle_t* root_handle,
    uint32_t offset)
{
    // Superblock suchen
    if (fseek(root_handle->device, offset, SEEK_SET) != 0)
    {
        return -1;
    }

    // Superblock schreiben
    if (fwrite(&root_handle->superblock, 1, sizeof(root_handle->superblock),
        root_handle->device) != sizeof(root_handle->superblock))
    {
        return -2;
    }
    
    return 0;
}

/**
 * Prueft ob eine Zahl ein vielfaches von einer anderen ist
 *
 * @param x Zahl von der in Erfahrung gebracht werden soll ob sie ein
 *          viellfaches von y ist oder nicht.
 * @param y
 *
 * @return TRUE wenn die Zahl ein Vielfaches ist, FALSE sonst
 */
static bool ext2_test_root(uint32_t x, uint32_t y)
{
    uint32_t current = y;
    
    // Jetzt wird current solange mit y multipliziert bis das Ergebnis groesser
    // oder gleich wie x ist
    while (current < x) {
        current *= y;
    }

    // Wenn das current jetzt gleich gross ist wie x ist es ein Vielfaches von
    // y sonst nicht.
    if (current == x) {
        return TRUE;
    } 
    return FALSE;
}

/**
 * Prueft ob eine bestimmte Blockgruppe ein Superblockbackup enthaelt oder
 * nicht.
 *
 * @param num Blockgruppennummer
 *
 * @return TRUE falls sie einen enthaelt, FALSE im Fehlerfall
 */
static bool ext2_superblock_in_bg(ext2_root_handle_t* root_handle,
    uint16_t num)
{
    // Falls das SPARSE_SUPER-Flag nicht gesetzt ist beinhaltet jede
    // Blockgruppe eine Kopie
    if (ext2_feature_rocomp(root_handle, EXT2_FEATURE_ROCOMP_SPARSE_SUPER)) {
        return TRUE;
    }

    // Mit SPARSE_SUPER ist die Entscheidung etwas komplizierter. Da werden
    // naemlich Kopien in allen Gruppen abgespeichert, deren Nummer ein
    // Vielfaches von 3, 5 oder 7 sind. Auch in der Ersten Gruppe ist dann eine
    // Kopie
    if ((num <= 1) || ext2_test_root(num, 3) || ext2_test_root(num, 5) ||
        ext2_test_root(num, 7))
    {
        return TRUE;
    }
    return FALSE;
}

/**
 * Superblock speichern
 *
 * @return 0 Bei Erfolg, im Fehlerfall einer der Fehlercodes:
 *      -1: Fehler beim Seek
 *      -2: Fehler beim Schreiben
 *      -3: Nur Lesezugriff
    */
int ext2_superblock_write(ext2_root_handle_t* root_handle)
{
    // Wenn das Dateisystem nur gelesen werden darf, muss abgebrochen werden
    if (root_handle->read_only == TRUE) {
        print_error("Superblock darf nicht gespeichert werden, da nur "
            "Lesezugriff erlaubt ist!");
        return -4;
    }
    
    // An Offset 1024 muss er manuell gespeichert werden, da er hier
    // unabhaengig von der Blockgroesse hin muss.
    int result = ext2_superblock_write_to(root_handle, 1024);
    if (result != 0) {
        return result;
    }

    // Anzahl der Blockgruppen errechnen
    uint32_t blockgroup_count = root_handle->group_count;
    
    // Jetzt wird noch in die noetigen Blockgruppen gespeichert
    uint32_t blockgroup;
    for (blockgroup = 1; blockgroup < blockgroup_count; blockgroup++) {
        // Wenn die aktuelle Blockgruppe ein Backup enthaelt wird es
        // gespeichert
        if (ext2_superblock_in_bg(root_handle, blockgroup) == TRUE) {
            // Blocknummer berechnen
            uint32_t block = root_handle->superblock.first_data_block +
                blockgroup * root_handle->superblock.blocks_per_group;
            
            // Kopie speichern
            result = ext2_superblock_write_to(root_handle, block *
                ext2_block_size(root_handle));
            if (result != 0) {
                return result;
            }
        }
    }

    return 0;
}
