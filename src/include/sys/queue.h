/*
 * Copyright (c) 2016 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _SYS_QUEUE_H
#define _SYS_QUEUE_H

#include <stddef.h>

#define SLIST_ENTRY(type) \
    struct { struct type* next; }

#define SLIST_HEAD(head_name, type) \
    struct head_name { struct type* first; }

#define SLIST_INIT(head) \
    do { head->first = NULL; } while (0)

#define SLIST_HEAD_INITIALIZER(head) \
    { .first = NULL }

#define SLIST_INSERT_AFTER(old_entry, new_entry, link) \
    do { \
        (new_entry)->link.next = (old_entry)->link.next; \
        (old_entry)->link.next = (new_entry); \
    } while (0)

#define SLIST_INSERT_HEAD(head, new_entry, link) \
    do { \
        (new_entry)->link.next = (head)->first; \
        (head)->first = (new_entry); \
    } while (0)

#define SLIST_FOREACH(var, head, link) \
    for ((var) = (head)->first; (var) != NULL; (var) = (var)->link.next)

#define SLIST_FOREACH_SAFE(var, head, link) \
    for (typeof(var) next = NULL, (var) = (head)->first; \
         (var) != NULL && (next = (var)->link.next, 1); \
         (var) = (next))

#endif
