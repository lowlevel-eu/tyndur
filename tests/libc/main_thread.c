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
        default:
            printf("* ERROR Unbekannter Testfall\n");
            break;
    }

    quit_qemu();
}
