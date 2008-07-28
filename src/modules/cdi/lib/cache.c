/*
 * Copyright (c) 2008 Antoine Kaufmann
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <lostio.h>

#include "cdi.h"
#include "cdi/misc.h"
#include "cdi/cache.h"

#define INVOFFSET (~0L)

static uint64_t cache_time = 0;
static cdi_list_t cache_list = NULL;

/**
 * Cache-Handle mit OS-spezifischen Daten
 */
struct cache {
    /** Eigentliches Cache-Handle */
    struct cdi_cache    cache;

    /** Anzahl der Blocks */
    size_t              block_count;

    /** Blockliste (sortiert nach offset) */
    cdi_list_t          block_list;

    /** Liste mit Cache-Eintraegen */
    cdi_list_t          entry_list;

    /** Datenquelle */
    FILE*               device;
};

/**
 * Cache-eintrag
 */
struct entry {
    /** Eigentlicher CDI-Cache-Eintrag */
    struct cdi_cache_entry  entry;

    /** Offset */
    uint64_t                offset;

    /** Groesse des Bereichs */
    size_t                  size;

    /** Anzahl der Blocks */
    size_t                  block_count;

    /** Pointer auf ein Array mit Blockpointern */
    struct block**          blocks;

    /** Gesperrt */
    int                     locked;
};

/**
 * Block im Cache
 */
struct block {
    /** Offset auf dem Datentraeger */
    uint64_t    offset;

    /** Zeiger auf die Daten */
    void*       data;

    /** Anzahl der Eintraege, die diesen Block beinhalten */
    size_t      ref_count;

    /** Zeit der letzten Benutzung */
    uint64_t    access_time;

    /** Wird auf 1 gesetzt, wenn die Daten veraendert wurden */
    int         dirty;
};

struct cdi_cache* cache_create(FILE* device)
{
    struct cache* cache = malloc(sizeof(*cache));
    size_t i;

    cache->device = device;

    // FIXME
    cache->cache.block_size = 16 * 1024;
    cache->block_count = 8 * 1024 * 1024 / cache->cache.block_size;

    cache->entry_list = cdi_list_create();
    cache->block_list = cdi_list_create();
    for (i = 0; i < cache->block_count; i++) {
        struct block* block = malloc(sizeof(struct block));
        block->offset = INVOFFSET;
        block->dirty = 0;
        block->data = malloc(cache->cache.block_size);

        cdi_list_push(cache->block_list, block);
    }

    // Cache in die Liste eintragen damit er Synchronisiert wird
    if (cache_list == NULL) {
        cache_list = cdi_list_create();
    }
    cdi_list_push(cache_list, cache);

    return (struct cdi_cache*) cache;
}

/**
 * Einzelnen Block auf das Geraet schreiben. Das wird nur durchgefuehrt, wenn er
 * veraendert wurde.
 *
 * @param c Cache-Handle
 * @param b Block-Pointer
 *
 * @return 1 bei Erfolg, 0 im Fehlerfall
 */
static inline int cache_sync_block(struct cache* c, struct block* b)
{
    if (!b->dirty) {
        return 1;
    }

    lio_seek(c->device, b->offset, SEEK_SET);
    if (fwrite(b->data, 1, c->cache.block_size, c->device) != c->cache.
        block_size)
    {
        puts("cdi_cache: Schreiben beim Cache-Sync fehlgeschlagen.");
        return 0;
    }
    b->dirty = 0;

    return 1;
}

/**
 * Veraenderte Blocks aus dem Cache auf das Geraet schreiben
 *
 * @param cache Handle
 *
 * @return 1 bei Erfolg, 0 im Fehlerfall
 */
static int cache_sync(struct cache* cache)
{
    struct cache* c = (struct cache*) cache;
    struct block* b;
    size_t i;

    for (i = 0; (b = cdi_list_get(c->block_list, i)); i++) {
        // Sobald ein Eintrag mit INVOFFSET erreicht wird, ist das Durchsuchen
        // beendet
        if (b->offset == INVOFFSET) {
            return 1;
        }

        cache_sync_block(c, b);
    }

    return 1;
}

/**
 * Caches Synchronisieren
 */
void caches_sync_all()
{
    size_t i;
    struct cache* c;

    if (cache_list == NULL) {
        return;
    }

    for (i = 0; (c = cdi_list_get(cache_list, i)); i++) {
        if (!cache_sync(c)) {
            puts("cdi_cache: Fehler beim Syncen der Caches.");
        }
    }
}

/**
 * Block in der Liste finden
 *
 * @param cache Handle
 * @param block Blocknummer
 *
 * @return Pointer auf den Block oder NULL wenn er nicht in der Liste ist
 */
static struct block* block_find(struct cache* cache, uint64_t block)
{
    size_t i;
    struct block* b;
    size_t block_size = cache->cache.block_size;

    // TODO: Geht auch noch effizienter, da die Liste sortiert ist
    for (i = 0; (b = cdi_list_get(cache->block_list, i)) && (b->offset !=
        INVOFFSET); i++)
    {
        if (block * block_size == b->offset) {
            return b;
        }
    }

    return NULL;
}

/**
 * Block im Cache finden, der ersetzt werden kann
 *
 * @param cache Handle
 * @param index Pointer auf Speicherstelle, an der der Listenindex
 *              abgespeichert werden soll.
 *
 * @return 1 wenn ein Element gefunden wurde, 0 sonst
 */
static size_t block_allocate(struct cache* cache, size_t* index)
{
    struct block* b;
    size_t i;
    size_t lru_index = 0;
    uint64_t lru_time = cache_time + 1;

    // Wenn noch unbenutzte Blocks existieren, gehoert der letzte sicher dazu,
    // weil die Liste sortiert ist
    b = cdi_list_get(cache->block_list, cdi_list_size(cache->block_list) - 1);

    if (b->offset == INVOFFSET) {
        *index = cdi_list_size(cache->block_list) - 1;
        return 1;
    }


    // Sonst wird nach LRU ein Element zum ersetzen gesucht
    for (i = 0; (b = cdi_list_get(cache->block_list, i)); i++) {
        if (b->ref_count != 0) {
            continue;
        }

        if (b->access_time < lru_time) {
            lru_index = i;
            lru_time = b->access_time;
        }
    }

    // Wenn kein Element gefunden wurde....
    if (lru_time == cache_time + 1) {
        return 0;
    }

   // Aenderungen auf das Geraet schreiben, bevor der Block aus der Liste
    // geluescht wird.
    cache_sync_block(cache, cdi_list_get(cache->block_list, lru_index));

    *index = lru_index;
    return 1;
}

/**
 * Neuen block laden, der noch nicht in der Liste ist
 *
 * @param cache Handle
 * @param block Blocknummer
 * @param read  Wenn 1 dann muss der Block eingelesen werden, bei 0 wird nur
 *              ein freier Block herausgesucht (Wenn er eh ganz ueberschrieben
 *              wird)
 *
 * @return Pointer auf den Block
 */
static struct block* block_load(struct cache* cache, uint64_t block, int read)
{
    size_t index;
    struct block* b;
    struct block* b_tmp;
    size_t block_size = cache->cache.block_size;
    uint64_t offset = block * block_size;
    size_t i;

    if (!block_allocate(cache, &index)) {
        puts("cdi_cache Panic: Kein Unbenutztes Element gefunden");
        return NULL;
    }

    b_tmp = cdi_list_get(cache->block_list, index);
    b = cdi_list_remove(cache->block_list, index);

    // Daten des neuen Blocks einlesen
    b->offset = offset;

    if (read) {
        lio_seek(cache->device, offset, SEEK_SET);
        if (fread(b->data, 1, block_size, cache->device) != block_size) {
            puts("cdi_cache Panic: Einlesen der Daten fehlgeschlagen");
            return NULL;
        }
    }

    // TODO: Auch hier gehts effizienter
    for (i = 0; (b_tmp = cdi_list_get(cache->block_list, i)) && (b_tmp->offset <
        offset); i++);

    cdi_list_insert(cache->block_list, i, b);
    return b;
}

/**
 * Block holen; Falls er noch nicht in der Liste ist, wird er geladen
 *
 * @param cache Handle
 * @param block Blocknummer
 * @param read  Wenn 1 dann muss der Block eingelesen werden, bei 0 wird nur
 *              ein freier Block herausgesucht (Wenn er eh ganz ueberschrieben
 *              wird)
 *
 *
 * @return Pointer auf den Block
 */
static struct block* get_block(struct cache* cache, uint64_t block, int read)
{
    struct block* b;

    if (!(b = block_find(cache, block))) {
        b = block_load(cache, block, read);
    }

    if (b) {
        b->access_time = ++cache_time;
    }


    return b;
}

/**
 * Cache-Eintrag erstellen
 *
 * @param offset    Position auf dem Datentraeger
 * @param size      Groesse des Bereichs
 *
 * @return Handle
 */
struct cdi_cache_entry* cdi_cache_entry_new(struct cdi_cache* cache,
    uint64_t offset, size_t size)
{
    struct entry* entry = malloc(sizeof(struct entry));
    size_t block_size = cache->block_size;
    uint64_t start_block = offset / block_size;
    uint64_t end_block = (offset + size) / block_size;

    entry->entry.cache = cache;
    entry->locked = 0;
    entry->offset = offset;
    entry->size = size;
    entry->block_count = end_block - start_block + 1;
    entry->blocks = calloc(sizeof(struct block*) * entry->block_count, 1);

    return (struct cdi_cache_entry*) entry;
}

/**
 * Cache-Eintrag freigeben
 *
 * @param entry Handle
 */
void    cdi_cache_entry_release(struct cdi_cache_entry* entry)
{
    struct entry* e = (struct entry*) entry;
    size_t i;

    if (e->locked) {
        cdi_cache_entry_unlock(entry);
    }

    for (i = 0; i < e->block_count; i++) {
        struct block* block = e->blocks[i];
        if (block) {
            block->ref_count--;
        }
    }
}

/**
 * Cache-Eintrag sperren
 *
 * @param entry Handle
 */
void    cdi_cache_entry_lock(struct cdi_cache_entry* entry)
{
    struct entry* e = (struct entry*) entry;
    e->locked = 1;
}

/**
 * Cache-Eintrag entsperren
 *
 * @param entry Handle
 */
void    cdi_cache_entry_unlock(struct cdi_cache_entry* entry)
{
    struct entry* e = (struct entry*) entry;
    e->locked = 0;
}

/**
 * Cache-Eintrag als veraendert markieren
 *
 * @param entry Handle
 */
void    cdi_cache_entry_dirty(struct cdi_cache_entry* entry)
{
    struct entry* e = (struct entry*) entry;
    size_t i;

    for (i = 0; i < e->block_count; i++) {
        struct block* block = e->blocks[i];
        block->dirty = 1;
    }
}



/**
 * Pointer auf einen Block im Cache-Eintrag holen
 *
 * @param entry Handle
 * @param block Blocknummer relativ zum Eintrag (der Offset innerhalb des
 *              Eintrags wird nicht beruecksichtigt)
 *
 * @return Pointer auf den Block
 */
void*   cdi_cache_entry_block(struct cdi_cache_entry* entry, size_t block)
{
    struct entry* e = (struct entry*) entry;

    if (block >= e->block_count) {
        return NULL;
    }

    return e->blocks[block]->data;
}

/**
 * Daten aus dem Cache-Eintrag lesen
 *
 * @param entry     Handle
 * @param offset    Offset relativ zum Cache-Eintrag
 * @param size      Groesse des zu lesenden Bereichs
 * @param buffer    Puffer in dem die Daten abgelegt werden sollen
 */
int     cdi_cache_entry_read(struct cdi_cache_entry* entry, size_t offset,
            size_t size, void* buffer)
{
    struct entry* e = (struct entry*) entry;
    size_t block_size = entry->cache->block_size;
    uint64_t dev_start_block = (e->offset + offset) / block_size;
    size_t start_block = (e->offset + offset) / block_size - e->offset /
        block_size;
    size_t end_block = (e->offset + offset + size -1) / block_size - e->offset /
        block_size;
    size_t block_count = end_block - start_block + 1;
    size_t i;
    size_t bytes_done = 0;

    // Nicht ueber den Eintragsrand hinauslesen
    if ((offset + size) > e->size) {
        return 0;
    }

    // Offset vom Block-Anfang im Cacheeintrag beruecksichtigen, falls vorhanden
    offset += e->offset % block_size;

    for (i = 0; i < block_count; i++) {
        void* block;
        size_t bytes = block_size;
        size_t off = 0;

        if (e->blocks[start_block + i] == NULL) {
            e->blocks[start_block + i] = get_block((struct cache*) entry->cache,
                dev_start_block + i, 1);
        }

        block = e->blocks[start_block + i]->data;
        e->blocks[start_block + i]->access_time = ++cache_time;

        // Beim ersten Durchlauf muss der Anfangsoffset beruecksichtigt werden
        if (i == 0) {
            off = offset % block_size;
            bytes = block_size - off;
        }

        // Beim letzten Durchlauf muss unter umstaenden nicht mehr ein ganzer
        // Block gelesen werden.
        if (i == block_count - 1) {
            bytes = size - bytes_done;
        }

        memcpy((void*) ((uintptr_t) buffer + bytes_done), (void*)
            ((uintptr_t) block + off), bytes);
        bytes_done += bytes;
    }

    return 1;
}

/**
 * Daten in den Cache-Eintrag schreiben
 *
 * @param entry     Handle
 * @param offset    Offset relativ zum Cache-Eintrag
 * @param size      Groesse des zu schreibenden Bereichs
 * @param buffer    Puffer aus dem die Daten gelesen werden
 */
int     cdi_cache_entry_write(struct cdi_cache_entry* entry, size_t offset,
            size_t size, const void* buffer)
{
    struct entry* e = (struct entry*) entry;
    size_t block_size = entry->cache->block_size;
    uint64_t dev_start_block = (e->offset + offset) / block_size;
    size_t start_block = (e->offset + offset) / block_size - e->offset /
        block_size;
    size_t end_block = (e->offset + offset + size - 1) / block_size - e->offset /
        block_size;
    size_t block_count = end_block - start_block + 1;
    size_t i;
    size_t bytes_done = 0;

    // Nicht ueber den Eintragsrand hinausschreiben
    if ((offset + size) > e->size) {
        return 0;
    }

    // Offset vom Block-Anfang im Cacheeintrag beruecksichtigen, falls vorhanden
    offset += e->offset % block_size;

    for (i = 0; i < block_count; i++) {
        void* block;
        size_t bytes = block_size;
        size_t off = 0;

        // Beim ersten Durchlauf muss der Anfangsoffset beruecksichtigt werden
        if (i == 0) {
            off = offset % block_size;
            bytes = block_size - off;
        }

        if (i == block_count - 1) {
            bytes = size - bytes_done;
        }


        if (e->blocks[start_block + i] == NULL) {
            int read = 1;

            if ((off == 0) && (bytes == block_size)) {
                read = 0;
            }

            e->blocks[start_block + i] = get_block((struct cache*) entry->cache,
                dev_start_block + i, read);
            e->blocks[start_block + i]->ref_count++;
        }

        block = e->blocks[start_block + i]->data;
        e->blocks[start_block + i]->access_time = ++cache_time;
        e->blocks[start_block + i]->dirty = 1;


        memcpy((void*) ((uintptr_t) block + off), (void*) ((uintptr_t) buffer +
            bytes_done), bytes);
        bytes_done += bytes;
    }

    return 1;
}


