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
#ifndef _CACHE_H_
#define _CACHE_H_
#include <types.h>

// Cache
typedef unsigned int cache_index_t;

/// Element im Cache
typedef struct {
    /// Index
    cache_index_t index;
    
    /// TRUE falls das Element benutzt wird, FALSE sonst
    bool used;

    /// TRUE wenn dieses Element veraendert wurde
    bool dirty;

    /// TRUE falls das Element gesperrt ist, weil es gerade benutzt wird
    bool locked;

    /// Daten
    void* data;

    /// Zeit an der das Element zuletzt benutzt wurde
    unsigned long long last_used;
} cache_element_t;

/// Handle fuer Caches
typedef struct {
    /// Anzahl der Elemente die der Cache beinhalten kann
    size_t element_count;
    
    /// Groesse eines Elements
    size_t element_size;

    /// Elemente
    cache_element_t* cache;

    /// In diesem Buffer werden die Daten der einzelnen Elemente verstaut
    void* data;
} cache_t;

cache_t* cache_create(size_t max_elements, size_t element_size);
void cache_foreach(cache_t* cache,
    bool (*handler) (cache_t*,cache_element_t*,void*), void* param,
    int filter_used, int filter_dirty, int filter_lock);
cache_element_t* cache_element_get(cache_t* cache, cache_index_t index,
    bool create);
void cache_element_unlock(cache_t* cache, cache_element_t* element);

#endif //ifndef _CACHE_H_

