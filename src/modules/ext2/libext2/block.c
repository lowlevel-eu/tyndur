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
 * Blockcache initialisieren
 */
bool ext2_block_init(ext2_root_handle_t* root_handle)
{
    root_handle->block_cache = cache_create(1024, ext2_block_size(root_handle));

    return (root_handle->block_cache != NULL);
}



struct sync_data {
    ext2_root_handle_t* root_handle;
    int status;
};
/**
 * Hilfsfunktion zum schreiben der Veraenderten Bloecken im Cache
 */
static bool ext2_block_sync_handler(cache_t* cache, cache_element_t* element,
    void* data)
{
    struct sync_data* sync_data = (struct sync_data*) data;
    ext2_root_handle_t* root_handle = sync_data->root_handle;
    size_t block_size = ext2_block_size(root_handle);

    // Daten schreiben
    if (fseek(root_handle->device, element->index * block_size, SEEK_SET) != 0)
    {
        sync_data->status = -1;
        return FALSE;
    }
    if (fwrite(element->data, 1, block_size, root_handle->device) !=
        block_size) 
    {
        sync_data->status = -2;
        return FALSE;
    }
    
    // Status wieder auf sauber setzen
    element->dirty = FALSE;
    return TRUE;  
}

/**
 * Blockcache synchronisieren
 */
int ext2_block_sync(ext2_root_handle_t* root_handle)
{
    struct sync_data sync_data = {
        .root_handle = root_handle,
        .status = 0
    };
    
    // Alle Blocks schreiben, die veraendert im Speicher liegen
    cache_foreach(root_handle->block_cache, ext2_block_sync_handler,
        &sync_data, 2, 2, 1);
    return sync_data.status;
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
    cache_element_t* element = cache_element_get(root_handle->block_cache,
        block, FALSE);
    void* load_dest;

    // Wenn das Element nicht im Cache ist, muss es vom Quelldatentraeger
    // geladen werden
    if (element == NULL) {
        element = cache_element_get(root_handle->block_cache, block, TRUE);
        // Wenn kein freies Element alloziert werden kann, muessen wir die
        // Daten direkt in den Puffer einlesen
        if (element == NULL) {
            load_dest = buffer;
        } else {
            load_dest = element->data;
        }
        
        // Daten einlesen
        if (fseek(root_handle->device, block * block_size, SEEK_SET) != 0) {
            if (element != NULL) {
                cache_element_unlock(root_handle->block_cache, element);
            }
            return -2;
        }
        if (fread(load_dest, 1, block_size, root_handle->device) !=
            block_size) 
        {
            if (element != NULL) {
                cache_element_unlock(root_handle->block_cache, element);
            }
            return -3;
        }
    } else {
        load_dest = element->data;
    }

    // Speicher kopieren
    if (load_dest != buffer) {
        memcpy(buffer, load_dest, block_size);
    }
    
    // Falls ein Cache-Element benutzt wurde, kann es entsperrt werden
    if (element != 0) {
        cache_element_unlock(root_handle->block_cache, element);
    }
    return 0;
}

/**
 * Einzelnen Block csheiben
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
    cache_element_t* element = cache_element_get(root_handle->block_cache,
        block, FALSE);

    // Wenn das Element nicht im Cache ist, werden die Daten direkt auf die
    // Platte geschrieben, sonst wird der Cache aktualisiert und das Element
    // als dirty Markiert.
    if (element == NULL) {
        if (fseek(root_handle->device, block * block_size, SEEK_SET) != 0) {
            return -1;
        }
        if (fwrite(buffer, 1, block_size, root_handle->device) != block_size) {
            return -2;
        }

        return 0;
    } else {
        memcpy(element->data, buffer, block_size);
        element->dirty = TRUE;
        cache_element_unlock(root_handle->block_cache, element);
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
    if (count >= 20) {
        //printf("sehr gross: %d\n", count);
        ext2_block_sync(root_handle);
        if (fseek(root_handle->device, block * block_size, SEEK_SET) != 0) {
            return -2;
        }
        if (fread(buffer, 1, block_size * count, root_handle->device) !=
            block_size * count) 
        {
            return -3;
        }
    } else {

        uint32_t b;
        void* ptr;
        for (b = 0; b < count; b++) {
            ptr = (void*) ((uintptr_t) buffer + b * block_size);
            if (ext2_block_read_single(root_handle, block + b, ptr) != 0) {
                return -1;
            }
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
 
    // Bei sehr grossen Leseanfragen wird der Cache umgangen, da sie vermutlich
    // nicht im Cache liegen, und ein zusammenhaengendes Schreiben wesentlich
    // schneller ist
    if (count >= 20) {
        //printf("sehr gross: %d\n", count);
        ext2_block_sync(root_handle);
        if (fseek(root_handle->device, block * block_size, SEEK_SET) != 0) {
            return -2;
        }
        if (fwrite(buffer, 1, block_size * count, root_handle->device) !=
            block_size * count) 
        {
            return -3;
        }
    } else {
        uint32_t b;
        void* ptr;
        for (b = 0; b < count; b++) {
            ptr = (void*) ((uintptr_t) buffer + b * ext2_block_size(
                root_handle));
            if (ext2_block_write_single(root_handle, block + b, ptr) != 0) {
                return -1;
            }
        }
    }
    return 0;
}

