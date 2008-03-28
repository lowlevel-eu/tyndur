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

#include "ext2.h"

/**
 * Blockgruppe suchen, in der der Block alloziert werden soll
 *
 * @return Nummer der Blockgruppe, oder -1 im Fehlerfall
 */
static int ext2_block_alloc_group(ext2_root_handle_t* root_handle)
{
    // Gefundene Gruppe
    int group = -1;
    
    // Die Gruppe mit den wenigsten Inodes wird gesucht
    int min = 0;
    int i;
    for (i = 0; i < root_handle->group_count; i++) {
        // Wenn eine bessere Gruppe gefunden wurde, wird die benutzt
        if (root_handle->group_descs[i].free_blocks > min) {
            min = root_handle->group_descs[i].free_blocks;
            group = i;
        }
    }
    return group;
}

/**
 * Blockbitmap einlesen
 *
 * @param group_num Gruppennummer deren Bitmap eingelesen werden soll
 * @param buffer Puffer in den die Bitmap gelesen werden soll
 *
 * @return 0 bei Erfolg, im Fehlerfall einer der Fehlercodes von block_read
 */
static inline int ext2_blockbitmap_read(ext2_root_handle_t* root_handle,
    int group_num, void* buffer)
{
    ext2_group_desc_t* group = &root_handle->group_descs[group_num];
    //puts("read block_bitmap");
    // Bitmap lesen
    return ext2_block_read(root_handle, group->block_bitmap, 1, buffer);
}

/**
 * Blockbitmap schreiben
 *
 * @param group_num Gruppennummer deren Bitmap geschrieben werden soll
 * @param buffer Puffer aus dem die Bitmap gelesen werden soll
 *
 * @return 0 bei Erfolg, im Fehlerfall einer der Fehlercodes von block_write
 */
static inline int ext2_blockbitmap_write(ext2_root_handle_t* root_handle,
    int group_num, void* buffer)
{
    ext2_group_desc_t* group = &root_handle->group_descs[group_num];
    //puts("write block_bitmap");
    // Bitmap lesen
    return ext2_block_write(root_handle, group->block_bitmap, 1, buffer);
}

/**
 * Unbenutzes Bit in Blockbitmap suchen
 *
 * @param bitmap Pointer auf die Bitmap
 * @param bitmap_size Groesse in Bytes
 * @param rec Empfehlung, die genommen wird, wenn der Block frei ist und
 *            use_rec != 0 ist
 * @param use_rec Bestimmt, ob der Empfohlene Block gueltig ist (0 =>
 *                ungueltig)
 *
 * @return Bit-Nummer
 */
static inline uint32_t ext2_blockbitmap_alloc(ext2_root_handle_t* root_handle,
    uint8_t* bitmap, size_t bitmap_size, uint32_t rec, int use_rec)
{
    int i;

    // Pruefen ob der gewuenschte Block frei ist
    if (use_rec && ((bitmap[rec / 8] & (1 << (rec % 8))) == 0)) {
        bitmap[rec / 8] |= (1 << (rec % 8));
        return  rec;
    }

    // Wenn ein ganzes dword frei ist, wird das genommen
    for (i = 0; i < bitmap_size; i++) {
        if (bitmap[i] == 0x0) {
            bitmap[i] |= 1;
            return  i * 8;
        }
    }

    // Bitmap durchsuchen nach einem freien Block
    for (i = 0; i < bitmap_size; i++) {
        // Wenn das Byte nicht total besetzt ist, wird das erste freie Bit
        // gesucht (=> Optimierungspotetial ;-))
        if (bitmap[i] == 0xFF) {
            continue;
        }

        int j;
        for (j = 0; j < 8; j++) {
            // Wenn das freie Bit gefunden wurde, wird es gesetzt und die Suche
            // wird abgebrochen
            if ((bitmap[i] & (1 << j)) == 0) {
                bitmap[i] |= (1 << j);

                return  i * 8 + j;
            }
        }
    }

    return 0;
}

/**
 * Bit in Blockbitmap loeschen
 *
 * @param bitmap Pointer auf die Bitmap
 * @param block Index relativ zum Gruppenanfang (0)
 *
 * @return 0 bei Erfolg, einer der Fehlercodes im Fehlerfall:
 *      -1: Ungueltiger index
 *      -2: Bit war nicht gesetzt
 */
static inline int ext2_blockbitmap_free(ext2_root_handle_t* root_handle,
    uint8_t* bitmap, uint32_t block)
{
    size_t bitmap_size = root_handle->superblock.blocks_per_group / 8;
    int index = block / 8;
    int bit = block % 8;
    
    // Pruefen ob der Block in der Bitmap liegt. Wenn nicht, wird abgebrochen
    if (index >= bitmap_size) {
        return -1;
    }
    
    // Wenn das Bit nicht gesetzt war, ist etwas schief gelaufen
    if ((bitmap[index] & (1 << bit)) == 0) {
        return -2;
    }

    // Bit loeschen
    bitmap[index] &= ~(1 << bit);
    
    return 0;
}

/**
 * Unbenutzen Block suchen
 *
 * @prev_block Nummer des vorherigen Blocks, um eine zusammenhaengende
 *             allokation zu ermoeglichen oder 0 falls dies nicht gewuenscht
 *             ist..
 *
 * @return Blocknummer bei Erfolg, im Fehlerfall 0
 */
uint32_t ext2_block_allocate(ext2_root_handle_t* root_handle,
    uint32_t prev_block)
{
    uint32_t block = 0;
    ext2_group_desc_t* group = NULL;
    int group_num;

    // Zuerst wird versucht, den Block zu nehmen, der dem vorherigen direkt
    // folgt.
    // Achtung: prev_block ist 1 zu gross, da die internen blocknummern bei 0
    //          anfangen. Aber falls der Wert 0 ist, darf noch nicht -1
    //          gerechnet werden, da der Wert sonst weiter unten unbrauchbar
    //          ist.

    if (prev_block != 0) {
        group_num = (prev_block - 1) / root_handle->superblock.
             blocks_per_group;
        group = &root_handle->group_descs[group_num];
        
        // Nur wenn in der Gruppe noch freie Blocks existieren kann sie
        // gewaehlt werden, sonst wird unten eine andere benutzt.
        // TODO: Eigentlich muesste hier geprueft werden, ob der gewuenschte
        // Block frei ist.
        if (root_handle->group_descs[group_num].free_blocks == 0) {
            group = NULL;
        }
    }

    // Ausfindig machen, in welcher Gruppe der Block am besten alloziert wird.
    if (group == NULL) {
        group_num = ext2_block_alloc_group(root_handle);

        if (group_num < 0) {
            print_error("Ungueltige Gruppe errechnet");
            return 0;
        }

        group = &root_handle->group_descs[group_num];
    }
    
    // Groesse der Bitmap errechnen, da der Letzte Block kleiner sein koennte
    size_t bitmap_size = root_handle->superblock.blocks_per_group / 8;
    if (group_num == root_handle->group_count - 1) {
        bitmap_size = (root_handle->superblock.block_count % root_handle->
            superblock.blocks_per_group) / 8;
    }

    // Blockbitmap laden
    uint8_t bitmap[ext2_block_size(root_handle)];
    if (ext2_blockbitmap_read(root_handle, group_num, bitmap) != 0) {
        print_error("Fehler beim Laden der Blockbitmap");
        return 0;
    }
    

    // Freien Block suchen (Wegen dem if siehe die Warnung weiter oben)
    if (prev_block != 0) {
        // Blocknummer umrechnen
        prev_block--;
        
        // Relativ zur Gruppe
        prev_block = (prev_block + 1) % root_handle->superblock.
            blocks_per_group;
        block = ext2_blockbitmap_alloc(root_handle, bitmap, bitmap_size,
            prev_block, 1);
    } else {
        block = ext2_blockbitmap_alloc(root_handle, bitmap, bitmap_size, 0,
            0);
    }

    // Wenn ein Block gefunden wurde, wird jetzt alles Noetige aktualisiert
    if (block != 0) {
        // Gruppendeskriptoren und Superblock aktuallisieren
        group->free_blocks--;
        root_handle->superblock.free_blocks--;

        // Blockbitmap aktualisieren
        if (ext2_blockbitmap_write(root_handle, group_num, bitmap) != 0) {
            print_error("Fehler beim Schreiben der Blockbitmap. Das fuehrt "
                "warscheinlich zu einem inkonsistenten Dateisystem!");
            return 0;
        }
        
        // Blocknummer anpassen
        block += group_num * root_handle->superblock.blocks_per_group + 1;
        //printf("\nBlockalloc: %d\n", block);
        // Wenn alles geklappt hat ist wir am Ende angelangt
        return block;
    }

    print_error2("Fehler beim allozieren eines Blocks: Laut Gruppendeskriptor "
        "sind noch Bloecke frei. Die Block-Bitmap widerspricht dem aber:",
        group->free_blocks);
    return 0;
}

/**
 * Block freigeben
 *
 * @param Blocknummer
 *
 * @return 0 bei Erfolg, im Fehlerfall einer der Fehlercodes:
 *      -1: Ungueltige Blocknummer
 *      -2: Fehler beim Laden der Bitmaps
 *      -3: Block nicht benuzt
 */
int ext2_block_free(ext2_root_handle_t* root_handle, uint32_t block)
{
    // Der erste Block ist in der Bitmap der 0.
    //printf("\nBlockfree: %d\n", block);
    block--;
    //printf("block: %d  block_count: %d\n", block, root_handle->superblock.block_count);
    // Pruefen ob es sich um einen gueltigen Block handelt
    if (block >= root_handle->superblock.block_count) {
        return -1;
    }

    // Gruppennummer errechnen
    int group_num = block / root_handle->superblock.blocks_per_group;
    //printf("\ngroup = %d  %d\n", group_num, root_handle->superblock.block_count / root_handle->
    //    superblock.blocks_per_group);
    if (group_num > root_handle->superblock.block_count / root_handle->
        superblock.blocks_per_group)
    {
        print_error("Ungueltige Gruppennummer errechnet. Entweder Superblock "
            "oder Blockgruppen-Deskriptoren beschaedigt.");
        return -7;
    }
    ext2_group_desc_t* group = &root_handle->group_descs[group_num];

    // Blockbitmap laden
    uint8_t bitmap[ext2_block_size(root_handle)];
    if (ext2_blockbitmap_read(root_handle, group_num, bitmap) != 0) {
        print_error("Fehler beim Laden der Blockbitmap");
        return -2;
    }
    

    // Blocknummer relativ zur Gruppe bitte
    block -= group_num * root_handle->superblock.blocks_per_group;

    // Versuchen den Block freizugeben
    int result = ext2_blockbitmap_free(root_handle, bitmap, block);
    if (result == -2) {
        print_error("Versuch freien Block frei zu geben!");
        return -3;
    } else if (result != 0) {
        print_error("Versuch ungueltigen Block frei zu geben!");
        return -1;
    }
    
    // Blockbitmap aktualisieren
    if (ext2_blockbitmap_write(root_handle, group_num, bitmap) != 0) {
        print_error("Fehler beim Schreiben der Blockbitmap. Das fuehrt "
            "warscheinlich zu einem inkonsistenten Dateisystem!");
        return -6;
    }
    
    // Gruppendeskriptor und Superblock aktuallisieren
    group->free_blocks++;
    root_handle->superblock.free_blocks++;

    // Wenn alles geklappt hat ist wir am Ende angelangt
    return 0;
}

