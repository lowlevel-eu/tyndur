/*
 * Copyright (c) 2011 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Andreas Freimuth.
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

#include "testlib.h"

#include <stdlib.h>
#include <stdio.h>

struct test_data {
    int     num;
    int*    sorted;
    int     lookups;
    int**   keys;
};

static const struct test_data test_data[] = {
    {
        .num    = 0,
        .sorted = (int[]) {},
        .lookups= 1,
        .keys   = (int*[]) {(int[]){42, -1},},
    },
    {
        .num    = 2,
        .sorted = (int[]) { 2, 5 },
        .lookups= 5,
        .keys   = (int*[]) {
            (int[]){2, 0},(int[]){5, 1},(int[]){3, -1}, (int[]){1, -1},
            (int[]){1337, -1}
        },
    },
    {
        .num    = 5,
        .sorted = (int[]) { -21, 0, 2, 5, 7 },
        .lookups= 7,
        .keys   = (int*[]) {
            (int[]){0, 1},(int[]){5, 3},(int[]){3, -1},(int[]){1, -1},
            (int[]){1337, -1},(int[]){7, 4},(int[]){-22, -1}
        },
    },
};

static int compare(const void* a, const void* b)
{
    return *(int*)a - *(int*)b;
}

int test_bsearch(void)
{
    int i,j;
    int num_tests = sizeof(test_data) / sizeof(test_data[0]);

    for (i = 0; i < num_tests; i++) {
        const struct test_data* t = &test_data[i];

        for (j = 0; j < t->lookups; j++) {
            int* pos = bsearch(&t->keys[j][0], t->sorted, t->num, sizeof(int), compare);
            if (pos == NULL && t->keys[j][1] == -1) {
                // printf("* PASS %s: Datensatz %d Lookup %d\n", test_name, i,j);
            } else if (pos - t->sorted == t->keys[j][1]) {
                // printf("* PASS %s: Datensatz %d Lookup %d\n", test_name, i,j);
            } else {
                printf("* FAIL %s: Datensatz %d Lookup %d\n", test_name, i,j);
                return -1;

            }
        }
        printf("* PASS %s: Datensatz %d\n", test_name, i);
    }

    return 0;
}
