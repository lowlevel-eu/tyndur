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
int ext2_block_init(ext2_root_handle_t* root_handle)
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
    {
        uint8_t buffer[block_size * block_count];
        uint32_t i;

        // Jetzt werden die Blocks in den Buffer kopiert
        for (i = 0; i < block_count; i++) {
            e = bc_get(cache, start + i);
            memcpy(buffer + (block_size * i), e->data, block_size);
        }

        // Daten schreiben
        if (root_handle->dev_write(root_handle, start * block_size, block_size
            * block_count, buffer) != block_size * block_count)
        {
            print_error("schreiben beim cache-sync fehlgeschlagen!");
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

