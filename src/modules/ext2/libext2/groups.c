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
 * Block errechnen, in dem die Gruppen-Deskriptorentabelle anfaengt
 */
static inline uint32_t ext2_get_gdt_start(ext2_root_handle_t* root_handle)
{
    // Normalerweise einfach nach dem Superblock kann mit erweiterungen aber
    // variieren
    return root_handle->superblock_block + 1;
}

/**
 * Block Guppen-Informationen in den Speicher laden
 *
 * @return 0 bei Erfolg, einer der folgenden Codes im Fehlerfall:
 *      -1: Bloecke konnten nicht gelesen werden
 *      -2: Nicht genug Speicher verfuegbao
 *      t_handle->group_descs
 */
int ext2_groups_init(ext2_root_handle_t* root_handle)
{
    uint32_t start_block = ext2_get_gdt_start(root_handle);

    // Anzahl der Blockgruppen berechnen. Diese ergibt sich aus der Anzahl der
    // Bloecke und der Anzahl der Bloecke pro Gruppe
    uint32_t group_count = root_handle->superblock.block_count / root_handle->
        superblock.blocks_per_group;
    if ((root_handle->superblock.block_count % root_handle->superblock.
        blocks_per_group) != 0)
    {
        group_count++;
    }
    
    //printf("Read groups: %d   %d\n", start_block, group_count);

    // So, jetzt noch die Anzahl der Bloecke berechnen, die durch die
    // Gruppendeskriptoren belgt werden
    uint32_t block_count = ext2_block_count(root_handle, group_count * sizeof(
        struct ext2_group_descriptor));

    // Puffer fuer die Bloecke erstellen
    uint8_t buffer[block_count * ext2_block_size(root_handle)];
    struct ext2_group_descriptor* group_descs = (struct ext2_group_descriptor*)
        buffer;

    // Bloecke einlesen
    if (ext2_block_read(root_handle, start_block, block_count, buffer) != 0) {
        return -1;
    }
    
    // Puffer fuer die Gruppendeskriptoren im root_handle allozieren
    root_handle->group_count = group_count;
    root_handle->group_descs = malloc(group_count * sizeof(
        struct ext2_group_descriptor));
    if (root_handle->group_descs == NULL) {
        return -2;
    }

    // Gruppendeskriptoren kopieren
    uint32_t i;
    for (i = 0; i < group_count; i++) {
        root_handle->group_descs[i] = group_descs[i];
    }
    return 0;
}

/**
 * Block Gruppen-Informationen auf den Datentraeger schreiben
 *
 * @return 0 bei Erfolg, einer der folgenden Fehlercodes im Fehlerfall:
 *      -1: Bloecke konnten nicht gelesen werden
 *      -2: Bloecke konnten nicht geschrieben werden
 */
int ext2_groups_write(ext2_root_handle_t* root_handle)
{
    uint32_t start_block = ext2_get_gdt_start(root_handle);
    uint32_t group_count = root_handle->group_count;
    //printf("Write groups: %d   %d\n", start_block, group_count);

    // Anzahl der Bloecke berechnen, die durch die Gruppendeskriptoren belegt
    // werden und passenden Buffer erstellen
    uint32_t block_count = ext2_block_count(root_handle, group_count * sizeof(
        struct ext2_group_descriptor));
    uint8_t buffer[block_count * ext2_block_size(root_handle)];
    struct ext2_group_descriptor* group_descs = (struct ext2_group_descriptor*)
        buffer;
    
    // Zuerst wird eingelesen, da ich nicht genau weiss, ob hinten noch was
    // sein koennte
    if (ext2_block_read(root_handle, start_block, block_count, buffer) != 0) {
        return -1;
    }

    // Gruppendeskriptoren kopieren
    uint32_t i;
    for (i = 0; i < group_count; i++) {
        //printf("group[%d].free_block =%d\n", i, group_descs[i].free_blocks);
        group_descs[i] = root_handle->group_descs[i];
    }

    // Jetzt wird geschrieben
    if (ext2_block_write(root_handle, start_block, block_count, buffer) != 0) {
        return -2;
    }
    
    return 0;
}

