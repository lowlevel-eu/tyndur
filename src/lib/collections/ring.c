/*
 * Copyright (c) 2010 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

/**
 * Wenn reader_pos und writer_pos gleich sind, ist der Ring vollstaendig
 * ausgelesen.
 */
struct ring_buffer {
    volatile size_t         writer_pos;
    volatile size_t         reader_pos;
    uint8_t                 data[];
};

#define RING_WRAPAROUND ((size_t) -1)

struct ring_entry {
    size_t                  size;
    uint8_t                 data[];
};

struct shared_ring {
    bool                    is_writer;
    size_t                  my_pos;
    struct ring_buffer*     buf;
    size_t                  buf_size;
};

/** Gibt den aktuellen Ringeintrag zurueck */
static inline struct ring_entry* ring_get(struct shared_ring* ring)
{
    return (struct ring_entry*) &ring->buf->data[ring->my_pos];
}

/**
 * Weist *buf einen Pointer auf den aktuellen Eintrag im Ring zu und gibt die
 * Laenge des Ringeintrags zurueck. Wenn kein neuer Eintrag vorhanden ist, wird
 * 0 zurueckgegeben.
 */
ssize_t ring_read_ptr(struct shared_ring* ring, void** buf)
{
    struct ring_entry* ent;
    size_t writer_pos = ring->buf->writer_pos;

    if (ring->is_writer) {
        return -EINVAL;
    }

    if (ring->my_pos > ring->buf_size - (sizeof(size_t))) {
        /*
         * Von 0 lesen, aber nur, wenn der Schreiber dabei nicht ueberholt wird
         */
        if (writer_pos < ring->my_pos) {
            ring->my_pos = 0;
            ring->buf->reader_pos = 0;
        } else {
            return 0;
        }
    }

    if (writer_pos == ring->my_pos) {
        return 0;
    }

    ent = ring_get(ring);
    if (ent->size == RING_WRAPAROUND) {
        ring->my_pos = 0;
        ring->buf->reader_pos = 0;

        if (writer_pos == ring->my_pos) {
            return 0;
        }

        ent = ring_get(ring);
    }

    *buf = ent->data;

    return ent->size;
}

/**
 * Schliesst eine Lesevorgang ab, d.h. passt die eigene Position an.
 */
int ring_read_complete(struct shared_ring* ring)
{
    struct ring_entry* ent;

    if (ring->is_writer) {
        return -EINVAL;
    }

    if (ring->buf->writer_pos == ring->my_pos) {
        return -EINVAL;
    }

    ent = ring_get(ring);

    ring->my_pos += ent->size;
    ring->buf->reader_pos = ring->my_pos;

    return 0;
}

/**
 * Weist *buf einen Pointer auf einen neuen Ringeintrag zu, der Platz fuer size
 * Bytes bietet. Wenn der Ring voll ist, wird -EBUSY zurueckgegeben.
 */
int ring_write_ptr(struct shared_ring* ring, void** buf, size_t size)
{
    struct ring_entry* ent;
    size_t reader_pos = ring->buf->reader_pos;
    size_t free_bytes;

    if (!ring->is_writer) {
        return -EINVAL;
    }

    /* Der Header muss auch noch mit rein */
    size += sizeof(size_t);

    /*
     * Falls der neue Eintrag nicht mehr in den Ring passt, muessen wir von
     * vorne anfangen. Der Leser darf dabei nicht ueberholt werden.
     */
    if (ring->my_pos + size >= ring->buf_size) {
        if (reader_pos > ring->my_pos || reader_pos == 0) {
            return -EBUSY;
        }

        if (ring->my_pos <= ring->buf_size - sizeof(size_t)) {
            ent = ring_get(ring);
            ent->size = RING_WRAPAROUND;
        }

        ring->my_pos = 0;
        ring->buf->writer_pos = 0;
    }

    /* Pruefen, ob genug Platz im Ring ist */
    if (ring->my_pos < reader_pos) {
        free_bytes = reader_pos - ring->my_pos - 1;
    } else {
        free_bytes = ring->buf_size - ring->my_pos - 1;
    }

    if (free_bytes < size) {
        return -EBUSY;
    }

    /* Eintrag anlegen */
    ent = ring_get(ring);
    ent->size = size;
    *buf = ent->data;

    return 0;
}

/**
 * Schliesst eine Schreibvorgang ab, d.h. passt die eigene Position an.
 */
int ring_write_complete(struct shared_ring* ring)
{
    struct ring_entry* ent;

    if (!ring->is_writer) {
        return -EINVAL;
    }

    ent = ring_get(ring);

    ring->my_pos += ent->size;
    ring->buf->writer_pos = ring->my_pos;

    return 0;
}

/**
 * Prueft, ob der Ring leer ist, d.h. der Ring keine Daten enthaelt, die noch
 * nicht gelesen worden sind.
 */
bool ring_empty(struct shared_ring* ring)
{
    return (ring->buf->writer_pos == ring->buf->reader_pos);
}

/**
 * Erstellt die Verwaltungsstrukturen fuer einen gemeinsamen Ringpuffer in
 * Shared Memory.
 */
static struct shared_ring* ring_init(void* buf, size_t size, bool is_writer)
{
    struct shared_ring* ring;

    if (size < sizeof(struct ring_buffer)) {
        return NULL;
    }

    ring = malloc(sizeof(*ring));
    if (ring == NULL) {
        return NULL;
    }

    ring->is_writer = is_writer;
    ring->my_pos    = 0;
    ring->buf       = buf;
    ring->buf_size  = size - sizeof(struct ring_buffer);

    if (is_writer) {
        memset(buf, 0, size);
        ring->buf->writer_pos = ring->my_pos;
    } else {
        ring->buf->reader_pos = ring->my_pos;
    }

    return ring;
}

struct shared_ring* ring_init_writer(void* buf, size_t size)
{
    return ring_init(buf, size, true);
}

struct shared_ring* ring_init_reader(void* buf, size_t size)
{
    return ring_init(buf, size, false);
}

void ring_free(struct shared_ring* ring)
{
    free(ring);
}
