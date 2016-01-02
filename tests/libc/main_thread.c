/*
 * Copyright (c) 2013 The tyndur Project. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <syscall.h>
#include <unistd.h>
#include <lock.h>

#include "testlib.h"

const char* test_name;

struct test1_data {
    int a;
    tid_t ctid;
};

static void test1_t1(void* arg)
{
    struct test1_data* data = arg;
    test_assert(data->a == 42);
    data->a = 43;
    data->ctid = get_tid();
    exit_thread();
    test_assert(false);
}

static void test1(void)
{
    tid_t tid, ctid;
    struct test1_data data = {
        .a = 42,
    };

    test_name = "sys_threads";
    tid = get_tid();

    ctid = create_thread((uint32_t) &test1_t1, &data);
    while (data.a == 42) {
        yield();
    }

    test_assert(data.a == 43);
    test_assert(data.ctid == ctid);
    test_assert(get_tid() == tid);

    printf("* PASS %s\n", test_name);
}

struct test2_data {
    uint32_t* bitfield;
    uint32_t  bit;
};

static mutex_t test2_mutex = 0;

static void test2_t1(void* arg)
{
    struct test2_data* data = arg;
    uint32_t* bitfield = data->bitfield;
    uint32_t bit = data->bit;

    while (true) {
        mutex_lock(&test2_mutex);
        *bitfield |= bit;
        yield();
        test_assert(*bitfield == bit);
        *bitfield &= ~bit;
        mutex_unlock(&test2_mutex);
    }
}

static void test2(void)
{
    uint32_t i;
    uint32_t bitfield = 0;
    uint64_t timeout;

    struct test2_data data[32];

    test_name = "mutex";

    for (i = 0; i < 2; i++) {
        data[i] = (struct test2_data) {
            .bitfield   = &bitfield,
            .bit        = 1 << i,
        };
        create_thread((uint32_t) &test2_t1, &data[i]);
    }

    /* sleep() benutzt RPC und das ist leider kaputt mit Threads */
    timeout = get_tick_count() + 2 * 1000000;
    while (get_tick_count() < timeout) {
        yield();
    }

    printf("* PASS %s\n", test_name);
}

static void test3_t1(void* arg)
{
    int* count = arg;
    void* buf;

    while ((*count)-- > 0) {
        buf = malloc(1234);
        free(buf);
    }

    *count = -1;
    exit_thread();
    test_assert(false);
}

static void test3(void)
{
    int count = 500000;
    int i;

    test_name = "malloc_mutex";

    for (i = 0; i < 64; i++) {
        create_thread((uint32_t) &test3_t1, &count);
    }
    while (count >= 0) {
        yield();
    }

    printf("* PASS %s\n", test_name);
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("* ERROR Zu wenige Parameter\n");
        quit_qemu();
    }

    switch (atoi(argv[1])) {
        case 1:
            test1();
            break;
        case 2:
            test2();
            break;
        case 3:
            test3();
            break;
        default:
            printf("* ERROR Unbekannter Testfall\n");
            break;
    }

    quit_qemu();
}
