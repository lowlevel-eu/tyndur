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

static void ext2_block_cache_write(block_cache_t* cache,
    bc_element_t* element);

/**
 * Blockcache initialisieren
 */
bool ext2_block_init(ext2_root_handle_t* root_handle)
{
    root_handle->block_cache = bc_create(1024, ext2_block_size(root_handle),
        ext2_block_cache_write);
    
    if (root_handle->block_cache == NULL) {
        return 0;
    }

    root_handle->block_cache->data = (void*) root_handle;
    return 1;
}

/**
 * Element um einen veraenderten Block aus dem Cache zu schreiben
 */
static void ext2_block_cache_write(block_cache_t* cache, bc_element_t* element)
{
    ext2_root_handle_t* root_handle = (ext2_root_handle_t*) cache->data;
    size_t block_size = cache->element_size;
    size_t block_count = 0;
    uint32_t start = element->index;
    bc_element_t* e = element;
    
    // Zaehlen, wieviele Blocks zusammenhaengend im Cache liegen
    do {
        block_count++;
        e = bc_get(cache, start + block_count);
    } while ((e != NULL) && (e->flags.dirty));
    

    // Neuer Block, da ein Array erstellt werden muss, dessen Groesse erst
    // jetzt klar ist.
    // printf("Sync %d blocks, start %d\n", block_count, start);
    {
        uint8_t buffer[block_size * block_count];
        uint32_t i;

        // Jetzt werden die Blocks in den Buffer kopiert
        for (i = 0; i < block_count; i++) {
            e = bc_get(cache, start + i);
            memcpy(buffer + (block_size * i), e->data, block_size);
        }

        // Daten schreiben
        if (fseek(root_handle->device, start * block_size, SEEK_SET)
            != 0)
        {
            print_error("fseek beim cache-sync fehlgeschlagen!");
            return;
        }

        if (fwrite(buffer, 1, block_size * block_count, root_handle->device) !=
            block_size * block_count)
        {
            print_error("fwrite beim cache-sync fehlgeschlagen!");
            return;
        }

        // Alle Blocks auf unveraendert setzten
        for (i = 0; i < block_count; i++) {
            e = bc_get(cache, start + i);
            e->flags.dirty = 0;
        }
    }
}

/**
 * Blockcache synchronisieren
 */
int ext2_block_sync(ext2_root_handle_t* root_handle)
{
    return !bc_sync(root_handle->block_cache);
}

/**
 * Blockgroesse in einem FAT-Dateisystem
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
 * Block direkt von dem Quellmedium lesen
 */
// TODO

/**
 * Block direkt auf das quellmedium schreiben
 */
// TODO

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
        if (fseek(root_handle->device, block * block_size, SEEK_SET) != 0) {
            return -2;
        }
        if (fread(load_dest, 1, block_size, root_handle->device) !=
            block_size) 
        {
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

        if (fseek(root_handle->device, block * block_size, SEEK_SET) != 0) {
            return -1;
        }
        if (fwrite(buffer, 1, block_size, root_handle->device) != block_size) {
            return -2;
        }

        return 0;
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
    //printf("block_read(%u, %u)\n", block, count);
    //
    // Groesser als die Maximale Nummer von Bloecken gibts eh nicht
    if (block + count - 1 > root_handle->superblock.block_count) {
        printf("FAIL1   Maxblockcount: %d\n", root_handle->superblock.block_count);
        return -1;
    }
    uint32_t block_size = ext2_block_size(root_handle);
    // Bei sehr grossen Leseanfragen wird der Cache umgangen, da sie vermutlich
    // nicht im Cache liegen, und ein zusammenhaengendes Lesen wesentlich
    // schneller ist
    /*if (count >= 20) {
        uint32_t i;
        bc_element_t* element;

        printf("sehr gross: %d\n", count);
        
        if (fseek(root_handle->device, block * block_size, SEEK_SET) != 0) {
            return -2;
        }
        if (fread(buffer, 1, block_size * count, root_handle->device) !=
            block_size * count) 
        {
            return -3;
        }
        
        // Blocks in den Cache laden und Blocks die veraendert im Cache liegen
        // im Buffer ueberschreiben
        for (i = 0; i < count; i++) {
            element = bc_get(root_handle->block_cache, block + i);

            if ((element != NULL) && element->flags.dirty) {
                memcpy(buffer + block_size * i, element->data, block_size);
            } else {
                bc_insert(root_handle->block_cache, block + i, buffer +
                    block_size * i, 0, 0);
            }
        }
    } else {*/
        uint32_t b;
        void* ptr;
        for (b = 0; b < count; b++) {
            ptr = (void*) ((uintptr_t) buffer + b * block_size);
            if (ext2_block_read_single(root_handle, block + b, ptr) != 0) {
                return -1;
            }
        }
    //}
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
    uint32_t count, void* buffer)
{
    //printf("block_write(%d, %d)\n", block, count);
    uint32_t block_size = ext2_block_size(root_handle);

    // Wenn es sich um ein nur lesbares Dateisytem handelt darf nicht darauf
    // geschrieben werden
    if (root_handle->read_only == TRUE) {
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

