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
#include <stdio.h>
#include <stdbool.h>

#include "sleep.h"

struct param {
    int a;
    void* b;
};

void *test_a(void *p)
{
    while (true) {
        printf("%d\n", ((struct param *)p)->a);
    }

    return p;
}

void *test_b(void *p)
{
    while (true) {
        printf("%p\n", ((struct param *)p)->b);
    }

    return ((struct param *)p)->b;
}

void *test_c(void *p)
{
    while (true) {
        printf("Hallo, Welt\n");
    }

    return p;
}

int main(int argc, char *argv[])
{
    pthread_t a, b, c;

    struct param data = {
        .a = 1337,
        .b = (void *) 0xdeadc0de,
    };

    pthread_create(&a, NULL, test_a, &data);
    pthread_create(&b, NULL, test_b, &data);
    pthread_create(&c, NULL, test_c, &data);

    msleep(1000);

    printf("%d, %d, %d\n", a, b, c);

    return 0;
}
