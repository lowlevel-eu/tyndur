/*
 * Copyright (c) 2012 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Patrick Pokatilo.
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

#include <pthread.h>
#include <lock.h>
#include <errno.h>
#include <collections.h>
#include <unistd.h>
#include <syscall.h>
#include <stdlib.h>

typedef struct
{
    pid_t process;
    tid_t thread;
    pthread_t name;
} __pthread_name_binding_t;

list_t __pthread_name_bindings = { NULL, 0 };
unsigned int __pthread_ids[PTHREAD_THREADS_MAX / 32];
lock_t __pthread_ids_lock = LOCK_UNLOCKED;

pthread_t pthread_self()
{
    pid_t pid = getpid();
    tid_t tid = gettid();

    if (tid == -1) {
        return 0;
    }

    int i = 0;
    __pthread_name_binding_t *binding = 0;

    while ((binding = list_get_element_at(&__pthread_name_bindings, i))) {
        if ((binding->process == pid) && (binding->thread == tid)) {
            return binding->name;
        }
    }

    return 0;
}

int pthread_create(pthread_t *thread,
    __attribute__((unused)) const pthread_attr_t *attr,
    void *(*start_routine)(void *), void *arg)
{
    lock(&__pthread_ids_lock);

    unsigned int name = 0;
    int i, j;

    for (i = 0; i < PTHREAD_THREADS_MAX / 32; i++)
    {
        if (__pthread_ids[i] != 0xffffffff) {
            for (j = 0; j < 32; j++) {
                if ((__pthread_ids[i] & (1 << j)) == 0) {
                    __pthread_ids[i] |= 1 << j;
                    name = i * 32 + j;
                    goto found;
                }
            }
        }
    }
    unlock(&__pthread_ids_lock);
    return -EAGAIN;

found:
    unlock(&__pthread_ids_lock);

    tid_t tid = create_thread((uint32_t)start_routine, arg);
    if (tid == -1) {
        return -EAGAIN;
    }

    __pthread_name_binding_t *binding = malloc(sizeof(__pthread_name_binding_t));

    binding->process = getpid();
    binding->thread = tid;
    binding->name = name;

    list_push(&__pthread_name_bindings, binding);

    *thread = name;

    return 0;
}
