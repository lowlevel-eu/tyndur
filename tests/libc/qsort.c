/*
 * Copyright (c) 2011 The tyndur Project. All rights reserved.
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

#include "testlib.h"

#include <stdlib.h>
#include <stdio.h>

struct test_data {
    int     num;
    int*    data;
    int*    sorted;
};

static const struct test_data test_data[] = {
    {
        .num    = 0,
        .data   = (int[]) {},
        .sorted = (int[]) {},
    },
    {
        .num    = 2,
        .data   = (int[]) { 2, 5 },
        .sorted = (int[]) { 2, 5 },
    },
    {
        .num    = 2,
        .data   = (int[]) { 5, 2 },
        .sorted = (int[]) { 2, 5 },
    },
    {
        .num    = 5,
        .data   = (int[]) { 5, 2, 7, -21, 0 },
        .sorted = (int[]) { -21, 0, 2, 5, 7 },
    },
    {
        .num    = 6,
        .data   = (int[]) { 6, 2, 5, 4, 3, 2 },
        .sorted = (int[]) { 2, 2, 3, 4, 5, 6 },
    },
};

static int compare(const void* a, const void* b)
{
    return *(int*)a - *(int*)b;
}

static void dump_result(int* data, int num)
{
    int i;

    printf("* Ergebnis: ");
    for (i = 0; i < num; i++) {
        printf("%s%d", i ? ", " : "", data[i]);
    }
    printf("\n");
}

static int compare_result(int* data, int* sorted, int num)
{
    int i;

    for (i = 0; i < num; i++) {
        if (data[i] != sorted[i]) {
            return 0;
        }
    }

    return 1;
}

int test_qsort(void)
{
    int i;
    int num_tests = sizeof(test_data) / sizeof(test_data[0]);

    for (i = 0; i < num_tests; i++) {
        const struct test_data* t = &test_data[i];
        qsort(t->data, t->num, sizeof(int), compare);
        if (compare_result(t->data, t->sorted, t->num)) {
            printf("* PASS %s: Datensatz %d\n", test_name, i);
        } else {
            printf("* FAIL %s: Datensatz %d\n", test_name, i);
            dump_result(t->data, t->num);
        }
    }

    return 0;
}
