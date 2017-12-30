/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
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

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "kernel.h"
#include "lostio_int.h"
#include "kprintf.h"

static struct lio_service service;

static struct lio_tree tree = {
    .service = &service,
    .source = NULL,
};

struct pipehandle {
    /* Die Größe der Ressource wird automatisch von LIO angepasst, aber die
     * allozierte Puffergröße ist eine separate Sache. */
    uint64_t bufsize;
    uint8_t* data;
};

static int pipe_read(struct lio_resource* res, uint64_t offset, size_t bytes,
                     void* buf)
{
    struct pipehandle* ph = res->opaque;

    BUG_ON(res->size != ph->bufsize);
    if (offset > res->size) {
        return -EINVAL;
    } else if (offset == res->size) {
        return -EAGAIN;
    }

    bytes = MIN(bytes, res->size - offset);
    memcpy(buf, ph->data + offset, bytes);

    /* TODO Puffer verkleinern: Alles vor offset kann weggeworfen werden */

    return 0;
}

static int pipe_write(struct lio_resource* res, uint64_t offset,
    size_t bytes, void* buf)
{
    struct pipehandle* ph = res->opaque;

    BUG_ON(offset + bytes > res->size);
    if (offset + bytes > ph->bufsize) {
        uint64_t new_size;
        void* new_buf;

        new_size = offset + bytes;
        new_buf = realloc(ph->data, new_size);
        if (new_buf == NULL) {
            return -ENOMEM;
        }
        ph->bufsize = new_size;
        ph->data = new_buf;
        lio_resource_update_size(res, new_size);
    }

    memcpy(ph->data + offset, buf, bytes);

    return 0;
}

struct lio_resource* lio_create_pipe(void)
{
    struct lio_resource* res = lio_create_resource();
    struct pipehandle *ph = malloc(sizeof(*ph));

    *ph = (struct pipehandle) {
        .bufsize = 0,
        .data    = NULL,
    };

    res->tree = &tree;
    res->size = 0;
    res->blocksize = 1;
    res->readable = true;
    res->writable = true;
    res->moredata = true;
    res->opaque = ph;

    return res;
}

void lio_destroy_pipe(struct lio_resource* res)
{
    struct pipehandle* ph = res->opaque;
    free(ph->data);
    free(ph);
    lio_destroy_resource(res);
}

void pipe_close(struct lio_stream* s)
{
    BUG_ON(s->is_composite);

    /* Eigentlich müssen wir moredata = false nur setzen, wenn der schreibende
     * Stream geschlossen wird. Wir wissen an dieser Stelle aber nicht mehr, ob
     * das der lesende oder der schreibende Stream war. Das macht deswegen
     * nichts, weil moredata niemanden mehr interessiert, wenn der lesende
     * Stream geschlossen ist. */
    s->simple.res->moredata = false;

    if (s->simple.res->ref == 0) {
        lio_destroy_pipe(s->simple.res);
    }
}

static struct lio_service service = {
    .name = "internal_pipes",
    .lio_ops = {
        .read   = pipe_read,
        .write  = pipe_write,
        .close  = pipe_close,
    },
};
