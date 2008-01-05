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
#include <stdlib.h>
#include <stdio.h>
#include "cache.h"

/// Diese Zeit wird bei Cache-Operationen erhoeht. Sie sorgt dafuer, dass
/// erkannt werden kann, welche Elemente zuletzt benutzt wurden, ohne dass
/// dauernd Syscalls notwendig sind
unsigned long long cache_time = 0;

/**
 * Hilfsfunktion zum Initialisieren des Caches, sie wird an cache_foreach
 * uebergeben und setzt alle Elemente auf Unbenutzt, und initialisiert ihre
 * data-Variable.
 */
static bool cache_init_element(cache_t* cache, cache_element_t* element,
    void* data)
{
    void** data_ptr = (void**) data;

    element->used = FALSE;
    element->dirty = FALSE;
    element->locked = FALSE;

    // Data initialisieren
    element->data = *data_ptr;
    
    // Pointer fuer das Naechste Element anpassen
    *data_ptr = (void*) ((char*) *data_ptr + cache->element_size);
    return TRUE;
}

/**
 * Cache erstellen
 *
 * @param max_elements Anzahl der Elemente, die der Cache maximal fassen soll
 * @param element_size Groesse eines Elements
 *
 * @return Pointer auf Cache-Handle bei Erfolg, im Fehlerfall NULL
 */
cache_t* cache_create(size_t max_elements, size_t element_size)
{
    cache_t* cache = malloc(sizeof(cache_t));
    if (cache != NULL) {
        cache->element_count = max_elements;
        cache->element_size = element_size;

        // Cache-Speicher allozieren
        size_t cache_size = max_elements * sizeof(cache_element_t);
        cache->cache = malloc(cache_size);
        if (cache->cache == NULL) {
            free(cache);
            return NULL;
        }
        
        // Cache-Daten allozieren
        size_t data_size = max_elements * element_size;
        void* data_ptr = malloc(data_size);
        cache->data = data_ptr;
        cache_foreach(cache, cache_init_element, &data_ptr, 0, 0, 0);
    }
    return cache;
}

/**
 * Fuer jedes Element, das den Filterkriterien entspricht, die Handlerfunktion
 * aufrufen. Das wird solange gemacht, bis entweder alle Elemente abgearbeitet
 * wurden, oder die Handler-Funktion FALSE zurueck gibt.
 *
 * @param handler Handlerfunktion
 * @param param Zweiter Parameter der an die Handlerfunktion durchgereicht wird
 * @param filter_used Wenn 1 werden nur Unbenutzte Elemente genommen, bei 2 nur
 *                      benutzte und in allen anderen Faelle alle.
 * @param filder_dirty Wenn 1 werden nur saubere Elemente genommen, bei 2 nur
 *                      solche die als dirty markiert sind, fuer alle anderen
 *                      Werte alle.
 * @param filter_lock Wenn 1 werden nur ungesperrte Elemente genommen, bei 2
 *                      nur gesperrte, sonst alle
 */
void cache_foreach(cache_t* cache,
    bool (*handler) (cache_t*,cache_element_t*,void*), void* param,
    int filter_used, int filter_dirty, int filter_lock)
{
    int i;
    cache_element_t* element;
    for (i = 0; i < cache->element_count; i++) {
        element = &cache->cache[i];
        
        // Filter fuer das Used-Flag
        if ((filter_used == 1) && (element->used == TRUE)) {
            // Nur unbenutze Elemente gewunscht, das aktuelle wird aber benutzt
            continue;
        } else if ((filter_used == 2) && (element->used == FALSE)) {
            // Nur Benutzte elemente gewuenscht
            continue;
        }

        // Filter fuer Dirty-Flag
        if ((filter_dirty == 1) && (element->dirty == TRUE)) {
            // Nur saubere Elemente gewuenscht, das aktuelle ist aber nicht
            // sauber
            continue;
        } else if ((filter_dirty == 2) && (element->dirty == FALSE)) {
            // Nur nicht saubere Elemente gewuenscht
            continue;
        }
        
        // Filter fuer Lock-Flag
        if ((filter_lock == 1) && (element->locked == TRUE)) {
            continue;
        } else if ((filter_lock == 2) && (element->locked == FALSE)) {
            continue;
        }

        if (handler(cache, element, param) != TRUE) {
            return;
        }
    }
}


struct cache_search {
    cache_index_t index;
    cache_element_t* result;
};

/**
 * Handler fuer cache_foreach bei der Suche nach einem Element mit einer
 * bestimmten ID.
 */
static bool cache_search_handler(cache_t* cache, cache_element_t* element,
    void* data)
{
    struct cache_search* search = (struct cache_search*) data;
    
    // Wenn das Element gefunden wurde, muss nicht mehr weiter gesucht werden
    if (element->index == search->index) {
        search->result = element;
        return FALSE;
    }
    return TRUE;
}

/**
 * Element anhand des Indexes suchen
 */
static cache_element_t* cache_search(cache_t* cache, cache_index_t index)
{    
    struct cache_search search = {
        .index = index,
        .result = NULL
    };

    // Element suchen
    cache_foreach(cache, cache_search_handler, &search, 2, 0, 0);
    return search.result;
}

struct cache_alloc {
    cache_element_t* element;
    bool overwrite;
};


/**
 * Hilfsfunktion fuer cache_alloc_element
 */
static bool cache_alloc_handler(cache_t* cache, cache_element_t* element,
    void* data)
{
    struct cache_alloc* alloc = (struct cache_alloc*) data;

    // Wenn nicht ein bestehendes Element ueberschrieben werden soll, ist die
    // suche hier beendet
    if (alloc->overwrite == FALSE) {
        alloc->element = element;
        return FALSE;
    } else if ((alloc->element == NULL) || (element->last_used < alloc->
            element->last_used))
    {
        alloc->element = element;
    }

    return TRUE;
}

/**
 * Unbenutztes Element suchen
 */
static cache_element_t* cache_alloc_element(cache_t* cache)
{
    struct cache_alloc alloc = {
        .element = NULL,
        .overwrite = FALSE
    };

    // Element suchen
    cache_foreach(cache, cache_alloc_handler, &alloc, 1, 0, 0);
    
    // Falls kein Freies gefunden wurde, wird das aelteste verworfen
    if (alloc.element == NULL) {
        alloc.overwrite = TRUE;

        // Diesmal Gesperrte und nicht saubere Elemente nicht benutzen
        cache_foreach(cache, cache_alloc_handler, &alloc, 2, 1, 1);
    }
    
    return alloc.element;
}

/**
 * Versuchen ein Element in den Cache zu legen.
 */
cache_element_t* cache_element_get(cache_t* cache, cache_index_t index,
    bool create)
{
    // Pruefen, ob das Element vielleicht schon im Cache liegt
    cache_element_t* element = cache_search(cache, index);
    if ((element != NULL) && (element->locked == TRUE)) {
        // Wenn das Element gesperrt ist, haben wir ein Problem. ;-)
        element = NULL;
    } else if ((element == NULL) && (create == TRUE))  {
        // Falls kein Element mit dem Index gefunden wurde, wird jetzt ein
        // neues gesucht
        if ((element = cache_alloc_element(cache)) != NULL) {
            element->index = index;
            element->used = TRUE;
            element->dirty = FALSE;
        } else {
            printf("Fehler beim allozieren");
        }
    }

    // Wenn ein passendes Element gefunden wurde, wird es jetzt gesperrt und
    // die Zugriffszeit wird aktualisiert
    if (element != NULL) {
        // Element und Zeit aktualisieren
        element->locked = TRUE;
        element->last_used = ++cache_time;
    }

    return element;
}

/**
 * Element entsperren
 */
void cache_element_unlock(cache_t* cache, cache_element_t* element)
{
    element->locked = FALSE;
}

