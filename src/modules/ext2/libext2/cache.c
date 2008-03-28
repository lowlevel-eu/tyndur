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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "cache.h"

/**
 * Zugriffszeit in einem Element aktualisieren
 *
 * @param element Pointer auf das Element
 */
static inline void bc_touch(bc_element_t* element);

/**
 * Cache-Element zum ersetzen finden
 *
 * @param index Index des neuen Elements
 *
 * @return Array-Index des Elements das erstezt werden kann.
 */
static inline bc_index_t bc_find_replaceable(block_cache_t* cache,
    bc_index_t index);

/**
 * Sucht das Element mit dem Enstprechenden Index, oder falls keines mit
 * diesem Index existiert, wird die Stelle angegeben an der es stehen muesste
 *
 * @param index Index des gesuchten Elements
 * @param dest Pointer auf die Variable, in der der gefundene Index abgelegt
 *             werden soll.
 *
 * @return 1 bei Erfolg, 0 sonst
 */
static bc_index_t bc_search_position(block_cache_t* cache,
    bc_index_t index, bc_index_t* dest);



block_cache_t* bc_create(bc_index_t element_count, size_t element_size,
    void (* write_element) (struct block_cache* cache, bc_element_t* element))
{
    block_cache_t* cache;
    bc_index_t i;
    bc_element_t* element;
    
    if (element_count == 0) {
        return NULL;
    }

    cache = malloc(sizeof(block_cache_t));
    if (!cache) {
        return NULL;
    }

    cache->write_element = write_element;
    cache->element_count = element_count;
    cache->element_size = element_size;
    cache->elements = malloc(sizeof(bc_element_t) * element_count);
    if (!cache->elements) {
        free(cache);
        return NULL;
    }

    for (i = 0; i < element_count; i++) {
        element = cache->elements + i;
        
        element->index = CACHE_INDEX_INVALID;

        element->data = malloc(element_size*2);
        assert(element->data != NULL);

        element->last_access = 0;
        element->flags.locked = 0;
        element->flags.dirty = 0;
    }
    return cache;
}

void bc_destroy(block_cache_t* cache)
{
    if (!cache) {
        return;
    }

    free(cache->elements);
    free(cache);
}

static inline void bc_touch(bc_element_t* element)
{
    static uint64_t time = 0;
    element->last_access = ++time;
}

static inline bc_index_t bc_find_replaceable(block_cache_t* cache,
    bc_index_t index)
{
    bc_element_t* element = NULL;
    bc_index_t i;

    bc_index_t la_index = CACHE_INDEX_INVALID;

    for (i = 0; i < cache->element_count; i++) {
        element = cache->elements + i;
        
        // Pruefen ob das Element prinzipiell ersetzt werden darf, wenn nicht,
        // wird zum naechsten gesprungen
        if (element->flags.locked) {
            continue;
        }
        
        // Wenn bis jetzt noch kein passendes Element gefunden wurde, dann muss
        // es genommen werden. Andernfalls wird geprueft ob das element aelter
        // ist
        if ((la_index == CACHE_INDEX_INVALID) || (element->last_access <
            cache->elements[la_index].last_access)) 
        {
            la_index = i;
        }
    }
    
    // Falls das Element dirty ist, muss es gespeichert werden
    if ((la_index != CACHE_INDEX_INVALID) && (cache->elements[la_index].flags.
        dirty))
    {
        cache->write_element(cache, cache->elements + la_index);
        cache->elements[la_index].flags.dirty = 0;
    }
    return la_index;
}

static bc_index_t bc_search_position(block_cache_t* cache,
    bc_index_t index, bc_index_t* dest)
{
    bc_index_t top = cache->element_count - 1;
    bc_index_t bottom = 0;
    bc_index_t pos;
    
    // Pruefen ob index ueberhaupt im Array enthalten ist
    if (index < cache->elements[0].index) {
        *dest = 0;
        return 0;
    } else if (index > cache->elements[top].index) {
        *dest = top;
    }

    // Binaere Suche nach dem Element
    do {
        pos = bottom + (top - bottom) / 2;
        
        if (cache->elements[pos].index == index) {
            break;
        } else if (cache->elements[pos].index > index) {
            top = pos - 1;
        } else {
            bottom = pos + 1;
            // Falls hier gleich abgebrochen wird, muss pos auf ein Element
            // zeigen, das nicht groesser ist, als das Gesuchte
            pos = bottom;
        }
    } while (top >= bottom);

    if (pos >= cache->element_count) {
        pos = cache->element_count - 1;
    }

    *dest = pos;

    return (cache->elements[pos].index == index);
}

int bc_insert(block_cache_t* cache, bc_index_t index, void* data, int dirty,
    int lock)
{
    bc_index_t i;
    bc_index_t dest;
    bc_element_t* element;
    void* old_data;

    if (bc_get(cache, index) != NULL) {
        return 0;
    }
    
    if (index == CACHE_INDEX_INVALID) {
        return 0;
    }
    
    // Arrayelement das ersetzt werden soll
    i = bc_find_replaceable(cache, index);
    if (i == CACHE_INDEX_INVALID) {
        return 0;
    }
    //printf("cache: Replace element %lld by %lld\n", (unsigned long long) cache->elements[i].index, (unsigned long long) index);

    // Stelle finden, an der das Element im Array eingetragen werden muss.
    bc_search_position(cache, index, &dest);
    
    // Alter Pointer auf Daten sichern
    old_data = cache->elements[i].data;

    // Die anderen Elemente verschieben
    if (i < dest) {
        // Wenn wird die Elemente nach unten verschieben, muss das zielelement
        // kleiner sein, als das aktuelle
        if ((cache->elements[dest].index > index) && (dest > 0)) {
            dest--;
        }

        memmove(cache->elements + i, cache->elements + i + 1, (dest - i) *
            sizeof(bc_element_t));
    } else if (i > dest) {
        // Wenn wird die Elemente nach oben verschieben, muss das zielelement
        // groesser sein, als das aktuelle
        if ((cache->elements[dest].index < index) && (dest < cache->
            element_count - 1)) 
        {
            dest++;
        }

        memmove(cache->elements + dest + 1, cache->elements + dest, (i - dest) *
            sizeof(bc_element_t));
    }

    element = cache->elements + dest;
    element->index = index;
    element->data = old_data;
    element->flags.dirty = dirty;
    element->flags.locked = lock;
    bc_touch(element);

    // Daten Kopieren, wenn gewuenscht
    if (data != NULL) {
        memcpy(element->data, data, cache->element_size);
    }

    return 1;
}

bc_element_t* bc_get(block_cache_t* cache, bc_index_t index)
{
    bc_index_t i;
    bc_element_t* element;

    if (!bc_search_position(cache, index, &i)) {
        return NULL;
    }

    element = cache->elements + i;
    bc_touch(element);
    return element;
}

int bc_sync(block_cache_t* cache)
{
    bc_index_t i;
    bc_element_t* element;
    
    for (i = 0; i < cache->element_count; i++) {
        element = cache->elements + i;
        if ((element->index != CACHE_INDEX_INVALID) && (element->flags.dirty))
        {
            // Wenn das Element gesperrt ist, wird eine Warnung ausgegeben
            if (element->flags.locked) {
                printf("bc_sync: Element %llu mit index %llu ist gesperrt!\n",
                    (unsigned long long) i, (unsigned long long) element->
                    index);
                continue;
            }

            cache->write_element(cache, element);
            element->flags.dirty = 0;
        }
    }
    return 1;
}

#if 0
static void bc_cache_dump(block_cache_t* cache)
{
    bc_index_t i;

    printf("Cache dump:\n");
    for (i = 0; i < cache->element_count; i++) {
        printf("  An stelle %llu liegt index %llu mit daten 0x%llx l:%d d:%d\n",
            i, cache->elements[i].index, cache->elements[i].data, cache->elements[i].flags.locked, cache->elements[i].flags.dirty);
    }
}
#endif

