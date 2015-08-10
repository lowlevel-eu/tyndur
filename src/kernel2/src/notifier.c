/*
 * Copyright (c) 2015 The tyndur Project. All rights reserved.
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

#include <stdlib.h>
#include "notifier.h"

#include "kprintf.h"

struct notifier {
    notifier_cb_t       cb;
    void*               opaque;
    bool                once;
    struct notifier**   prev;
    struct notifier*    next;
};

struct notifier* notifier_add(struct notifier **list_head, notifier_cb_t cb,
                              void* opaque, bool once)
{
    struct notifier* n = malloc(sizeof(*n));

    *n = (struct notifier) {
        .cb     = cb,
        .opaque = opaque,
        .once   = once,
        .prev   = list_head,
        .next   = *list_head,
    };

    *list_head = n;
    if (n->next) {
        n->next->prev = &n->next;
    }

    return n;
}

void notifier_remove(struct notifier* n)
{
    if (n->next) {
        n->next->prev = n->prev;
    }
    *n->prev = n->next;
    free(n);
}

void notify(struct notifier *list_head)
{
    struct notifier* n = list_head;
    struct notifier* next;

    while (n) {
        next = n->next;
        n->cb(n->opaque);
        if (n->once) {
            notifier_remove(n);
        }
        n = next;
    }
}
