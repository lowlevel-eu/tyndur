/*
 * Copyright (c) 2008 The LOST Project. All rights reserved.
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
#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>

// Ungueltiger Indexwert
#define CACHE_INDEX_INVALID ((bc_index_t) -1)

typedef uint32_t bc_index_t;

typedef struct {
    /// Nummer des Elements
    bc_index_t index;

    void*               data;
    
    struct {
        /// Das Element wurde veraendert und darf deshalb nicht einfach so
        /// ersetzt werden (Wert 1).
        int             dirty   : 1;

        /// Das Element ist gerade gesperrt und darf somit nicht ersetzt
        /// werden.
        int             locked  : 1;
    } flags;

    /// Letzter Zugriff (sollte von aussen nicht veraendert werden)
    uint64_t            last_access;
} bc_element_t;


struct block_cache;
typedef struct block_cache {
    /// Anzahl der Elemente im Cache
    bc_index_t element_count;
    
    /// Groesse der Daten bei einem Element
    size_t element_size;

    /// Zeiger auf Array mit Elementen
    bc_element_t* elements;

    /// Zeiger der vom Aufrufer bentuzt werden kann um eigene Informationen zum
    /// Cache zu hinterlegen.
    void* data;

    /// Funktion zum zurueckspielen von veraenderten Elementen
    void (* write_element)(struct block_cache* cache, bc_element_t* element);
} block_cache_t;

/**
 * Cache initialisieren
 *
 * @param element_count Anzahl der Blocks, die der Cache beinhalten soll
 *
 * @return Pointer auf das Cache-Handle
 */
block_cache_t* bc_create(bc_index_t element_count, size_t element_size,
    void (* write_element) (struct block_cache* cache, bc_element_t* element));

/**
 * Cache zerstoeren
 */
void bc_destroy(block_cache_t* cache);

/**
 * Element aus dem Cache holen
 */
bc_element_t* bc_get(block_cache_t* cache, bc_index_t index);

/**
 * Element in den Cache legen
 */
int bc_insert(block_cache_t* cache, bc_index_t index, void* data, int dirty,
    int lock);

/**
 * Alle veraenderten Elemente zurueck schreiben
 */
int bc_sync(block_cache_t* cache);

/**
 * Element sperren
 */
static inline int bc_element_lock(bc_element_t* element, int locked)
{
    element->flags.locked = locked;
    return 1;
}

#endif

