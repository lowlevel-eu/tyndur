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
 * Gruppe suchen in der der Inode angelegt werden soll
 *
 * @return Gruppennummer >= 0 bei Erfolg, -1 im Fehlerfall
 */
static inline int ext2_inode_alloc_find_group(ext2_root_handle_t* root_handle)
{
    int group = -1;

    // Die Gruppe mit den wenigsten Inodes wird gesucht
    int min = 0;
    int i;
    for (i = 0; i < root_handle->group_count; i++) {
        // Wenn eine "bessere" Gruppe gefunden wird, wird die Anzahl der freien
        // Inodes und die Gruppennummer gespeichert
        if (root_handle->group_descs[i].free_inodes > min) {
            min = root_handle->group_descs[i].free_inodes;
            group = i;
        }
    }

    return group;
}

/**
 * Inode-Bitmap laden
 *
 * @param group_num Gruppennummer
 * @param buffer Puffer in den die Bitmap geladen werden soll
 *
 * @return 0 bei Erfolg, im Fehlerfall einer der Fehlercodes von block_read
 */
static int ext2_inodebitmap_read(ext2_root_handle_t* root_handle,
    int group_num, void* buffer)
{
    ext2_group_desc_t* group = &root_handle->group_descs[group_num];
    
    // Laden
    return ext2_block_read(root_handle, group->inode_bitmap, 1, buffer);
}

/**
 * Inode-Bitmap speichern
 *
 * @param group_num Gruppennummer
 * @param buffer Puffer aus dem die Bitmap geladen werden soll
 *
 * @return 0 bei Erfolg, im Fehlerfall einer der Fehlercodes von block_write
 */
static int ext2_inodebitmap_write(ext2_root_handle_t* root_handle,
    int group_num, void* buffer)
{
    ext2_group_desc_t* group = &root_handle->group_descs[group_num];
    
    // Schreiben
    return ext2_block_write(root_handle, group->inode_bitmap, 1, buffer);
}

/**
 * Freies Bit in Inode-Bitmap suchen
 *
 * @param bitmap Pointer auf Inode-Bitmap
 * @param inode_num Pointer auf Variable in der das Ergebnis gespeichert werden
 *                  soll
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
static uint32_t ext2_inodebitmap_alloc(ext2_root_handle_t* root_handle,
    uint8_t* bitmap, uint32_t* inode_num)
{
    size_t bitmap_size = root_handle->superblock.inodes_per_group / 8;

    // Bitmap nach einem freien Inode durchsuchen
    // FIXME: Kann hier darauf vertraut werden, dass die reservierten Inodes
    // alle als benutzt markiert sind?
    int i;
    for (i = 0; i < bitmap_size; i++) {
        // Wenn der eintrag nicht ganz besetzt ist, wird das erste freie Bit
        // gesucht
        if (bitmap[i] == 0xFF) {
            continue;
        }
        int j;
        for (j = 0; j < 8; j++) {
            // Wenn ein freies Bit gefunden worden ist, wird es gesetzt und
            // das Suchen beendet
            if ((bitmap[i] & (1 << j)) == 0) {
                bitmap[i] |= (1 << j);
                *inode_num = i * 8 + j;
                return 0;
            }
        }
    }

    return -1;
}

/**
 * Bit in Inode-Bitmap loeschen und so den somit freigeben
 *
 * @param bitmap Pointer auf die Bitmap
 * @param inode Inode-Nummer
 *
 * @return 0 bei Erfolg, im Fehlerfall einer der Fehlercodes:
 *      -1: Ungueltiger Index
 *      -2: Bit nicht gesetzt
 */
static inline int ext2_inodebitmap_free(ext2_root_handle_t* root_handle,
    uint8_t* bitmap, uint32_t inode)
{
    size_t bitmap_size = root_handle->superblock.inodes_per_group / 8;
    int index = inode / 8;
    int bit = inode % 8;
    
    // Pruefen ob es sich um eine gueltige Inode-Nummer handelt
    if (index >= bitmap_size) {
        return -1;
    }

    // Falls das Bit nicht gesetzt war, ist etwas schief gelaufen
    if ((bitmap[index] & (1 << bit)) == 0) {
        return -2;
    }
    
    // Wenn nich kann es jetzt geloescht werden
    bitmap[index] &= ~(1 << bit);

    return 0;
}

/**
 * Freien Inode finden
 *
 * @return Inode-Nummer oder 0 im Fehlerfall
 */
uint32_t ext2_inode_allocate(ext2_root_handle_t* root_handle)
{
    // Gruppe finden, in der der neue Inode gepeichert werden soll
    int group_num = ext2_inode_alloc_find_group(root_handle);
    if (group_num < 0) {
        print_error("Keine freien Inodes mehr");
        return 0;
    }
    ext2_group_desc_t* group = &root_handle->group_descs[group_num];
    //printf("inode_alloc in group %d\n", group_num);
    // Inode-Bitmap laden
    uint8_t bitmap[ext2_block_size(root_handle)];
    if (ext2_inodebitmap_read(root_handle, group_num, bitmap) != 0) {
        print_error("Fehler beim Laden der Inode-Bitmap");
        return 0;
    }
    
    // Jetzt wird der erste freie Inode gesucht
    uint32_t inode;
    int result = ext2_inodebitmap_alloc(root_handle, bitmap, &inode);
    
    // Wenn ein Inode gefunden wurde muss die Anzahl der Freien im
    // Gruppendeskriptor angepasst werden, und die Bitmap gespeichert werden
    if (result == 0) {
        // Gruppendeskriptoren und Superblock anpassen
        group->free_inodes--;
        root_handle->superblock.free_inodes--;

        // Bitmap abspeichern
        if (ext2_inodebitmap_write(root_handle, group_num, bitmap) != 0) {
            print_error("Fehler beim Abspeichern der Inode-Bitmap");
            return 0;
        }

        // Offset bedingt durch die Gruppe dazuzaehlen und noch +1 weil mit 1
        // angefangen wird zu zaehlen
        inode += root_handle->superblock.inodes_per_group * group_num;
        inode += 1;

        return inode;
    }
    
    print_error("Fehler beim Allozieren eines Inodes: Laut Gruppendeskriptor-"
        "Tabelle sollte noch mindestens 1 frei sein. Die Inode-Bitmap "
        "widerspricht dem aber!");
    return 0;
}

/**
 * Inode freigeben
 *
 * @param inode_num Inodenummer
 * @return 0 bei Erfolg, im Fehlerfall einer der Fehlercodes:
 *      -1: Ungueltige Inodenummer
 *      -2: Fehler beim Laden der Bitmaps
 *      -3: Inode nicht benuzt
 *      -4: Fehler beim Laden des Inodes
 *      -5: Fehler beim Freigeben der Blocks im Inode
 */
int ext2_inode_free(ext2_root_handle_t* root_handle, uint32_t inode_num)
{
    // Inode laden
    ext2_inode_t inode;
    if (ext2_inode_read(root_handle, inode_num, &inode) != 0) {
        return -4;
    }

    // Fuers arbeiten mit Inodes ist es wirklich unpraktisch, dass da mit 1
    // angefangen wird bei der Nummerierung
    inode_num--;

    // Pruefen ob es sich um einen gueltigen Inode handelt
    if (inode_num >= root_handle->superblock.inode_count) {
        return -1;
    }

    // Gruppennummer errechnen
    int group_num = inode_num / root_handle->superblock.inodes_per_group;
    ext2_group_desc_t* group = &root_handle->group_descs[group_num];

    // Inode-Bitmap laden
    uint8_t bitmap[ext2_block_size(root_handle)];
    if (ext2_inodebitmap_read(root_handle, group_num, bitmap) != 0) {
        print_error("Fehler beim Laden der Inode-Bitmap");
        return -2;
    }
    
    // Inode-Nummer relativ zur Gruppe
    inode_num -= group_num * root_handle->superblock.inodes_per_group;

    // Versuchen den Inode in der Bitmap freizugeben
    int result = ext2_inodebitmap_free(root_handle, bitmap, inode_num);
    if (result == -2) {
        print_error("Versuch einen nicht benutzen Inode frei zu geben!");
        return -3;
    } else if (result != 0) {
        print_error("Versuch einen ungueltigen Inode frei zu geben!");
        return -6;
    }    

    // Bitmap speichern
    if (ext2_inodebitmap_write(root_handle, group_num, bitmap) != 0) {
        print_error("Fehler beim Abspeichern der Inode-Bitmap");
        return 0;
    }
    
    // Gruppendeskriptor und Superblock anpassen
    group->free_inodes++;
    root_handle->superblock.free_blocks++;

    return 0;
}

