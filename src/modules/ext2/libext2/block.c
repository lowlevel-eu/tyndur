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
#include <assert.h>
#include "ext2.h"

/**
 * Blockgroesse in einem EXT2-Dateisystem
 *
 * @return Blockgroesse in Bytes
 */
int ext2_block_size(ext2_root_handle_t* root_handle)
{
    // Die Groesse ergibt sich folgendermassen:
    // 1024 x pow(2, (block_size aus Superblock))
    return 1024 * (1 << root_handle->superblock.block_size);
}

/**
 * Anzahl der Bloecke die Benoetigt werden, um eine bestimmte Anzahl Bytes zu
 * laden.
 *
 * @param size Anzahl der bytes
 *
 * @return Anzahl der Bloecke
 */
uint32_t ext2_block_count(ext2_root_handle_t* root_handle, uint32_t size)
{
    uint32_t blocks = size / ext2_block_size(root_handle);
    if (size % ext2_block_size(root_handle)) {
        blocks++;
    }

    return blocks;
}

/**
 * Einzelnen Block einlesen
 *
 * @param block Blocknummer
 * @param buffer Pointer auf den Buffer in den der Inhalt kopiert werden soll
 *
 * @return 0 bei Erfolg, im Fehlerfall < 0
 */
static inline int ext2_block_read_single(ext2_root_handle_t* root_handle,
    uint32_t block, void* buffer)
{
    size_t block_size = ext2_block_size(root_handle);
    bc_element_t* element = bc_get(root_handle->block_cache,
        block);
    void* load_dest;

    // Versuchen das Element in den Cache aufzunehmen, falls es noch nicht drin
    // ist.
    if (element == NULL) {
        if ((bc_insert(root_handle->block_cache, block, NULL, 0, 0)) &&
            (element = bc_get(root_handle->block_cache, block)))
        {
            load_dest = element->data;
        } else {
            load_dest = buffer;
        }
        
        // Daten einlesen
        if (root_handle->dev_read(root_handle, block * block_size, block_size,
            load_dest) != block_size)
        {
            print_error("Fehler beim Lesen");
            return -3;
        }
    } else {
        //printf("Read aus dem cache\n");
        load_dest = element->data;
    }

    // Speicher kopieren
    if (load_dest != buffer) {
        memcpy(buffer, load_dest, block_size);
    }

    return 0;
}

/**
 * Einzelnen Block schreiben
 *
 * @param block Blocknummer
 * @param buffer Pointer auf den Buffer in den der Inhalt kopiert werden soll
 *
 * @return 0 bei Erfolg, im Fehlerfall < 0
 */
static inline int ext2_block_write_single(ext2_root_handle_t* root_handle,
    uint32_t block, void* buffer)
{
    size_t block_size = ext2_block_size(root_handle);
    bc_element_t* element = bc_get(root_handle->block_cache, block);

    // Wenn das Element nicht im Cache ist, wird versucht, es in den Cache zu
    // legen. Sonst wird der Cache aktualisiert und das Element als dirty
    // Markiert.
    if ((element != NULL) || ((bc_insert(root_handle->block_cache, block, NULL,
        0, 0)) && (element = bc_get(root_handle->block_cache, block))))
    {
        //printf("Write in den cache\n");
        memcpy(element->data, buffer, block_size);
        element->flags.dirty = 1;
    } else {
        if (root_handle->dev_write(root_handle, block * block_size, block_size,
            buffer) != block_size) 
        {
            return -1;
        }
    }
    return 0;
}

/**
 * Block vom Laufwerk laden
 *
 * @param block Blocknummer
 * @param buffer Puffer wo der block hingelesen werden soll
 * @param count Anzahl der Bloecke
 *
 * @return 0 bei Erfolg, einer der folgenden Werte im Fehlerfall:
 *      -1: Ungueltige Blocknummer
 *      -2: Fehler beim Seek
 *      -3: Fehler beim Lesen
 */
int ext2_block_read(ext2_root_handle_t* root_handle, uint32_t block,
    uint32_t count, void* buffer)
{
    uint32_t b;
    void* ptr;
    bc_element_t* element;
    uint32_t blocks_left;
    uint32_t current_count;
    uint32_t i;
    uint32_t block_size;

    // Groesser als die Maximale Nummer von Bloecken gibts eh nicht
    if (block + count - 1 > root_handle->superblock.block_count) {
        printf("FAIL1   Maxblockcount: %d\n", root_handle->superblock.block_count);
        return -1;
    }

    block_size = ext2_block_size(root_handle);
    for (b = 0; b < count; b++) {
        ptr = (void*) ((uintptr_t) buffer + b * block_size);

        element = bc_get(root_handle->block_cache, block + b);
        // Wenn der Block im Cache ist, wird dieser benutzt
        if (element != NULL) {
            memcpy(ptr, element->data, block_size);
        } else {
            blocks_left = count - b;

            // Zaehlen wieviele von den folgenden Blocks auch nicht im Cache
            // liegen.
            current_count = 1;
            do {
                element = bc_get(root_handle->block_cache, block + b +
                    current_count);
                current_count++;
            } while ((current_count < blocks_left) && (element == NULL));
            current_count--;

            // Daten einlesen
            if (root_handle->dev_read(root_handle, (block + b) * block_size,
                block_size * current_count, ptr) != block_size * current_count)
            {
                print_error("Fehler beim Lesen");
                return -3;
            }
    
            // Daten in Cache eintragen
            for (i = 0; i < current_count; i++) {
                bc_insert(root_handle->block_cache, block + b + i, (void*)
                    ((uintptr_t) ptr + block_size * i), 0, 0);
            }
            
            b += current_count - 1;
        }
    }
    return 0;
}

/**
 * Block auf Laufwerk schreiben
 * 
 * @param block Blocknummer
 * @param count Anzahl der Bloecke
 * @param buffer Puffer woraus der block gelesen werden soll
 *
 * @return 0 bei Erfolg, einer der folgenden Werte im Fehlerfall:
 *      -1: Ungueltige Blocknummer
 *      -2: Fehler beim Seek
 *      -3: Fehler beim Schreiben
 *      -4: Dateisytem ist nur lesbar
 */
int ext2_block_write(ext2_root_handle_t* root_handle, uint32_t block,
    uint32_t count, const void* buffer)
{
    //printf("block_write(%d, %d)\n", block, count);
    uint32_t block_size = ext2_block_size(root_handle);

    // Wenn es sich um ein nur lesbares Dateisytem handelt darf nicht darauf
    // geschrieben werden
    if (root_handle->read_only) {
        print_error("Block kann nicht geschrieben werden, da auf das "
            "Dateisystem nur lesend zugegriffen werden darf");
        return -4;
    }

    // Groesser als die Maximale Nummer von Bloecken gibts eh nicht
    if (block + count - 1> root_handle->superblock.block_count) {
        return -1;
    }
 
    uint32_t b;
    void* ptr;
    for (b = 0; b < count; b++) {
        ptr = (void*) ((uintptr_t) buffer + b * block_size);
        if (ext2_block_write_single(root_handle, block + b, ptr) != 0) {
            return -1;
        }
    }
    return 0;
}

