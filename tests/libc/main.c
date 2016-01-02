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

#include <stdio.h>
#include <stdlib.h>
#include <services.h>
#include <unistd.h>

#include "testlib.h"

const char* test_name;

extern int test_qsort(void);
extern int test_bsearch(void);
extern int test_sprintf(void);
extern int test_stdio(void);
extern int test_time(void);

static void test1(void)
{
    test_name = "qsort";
    test_qsort();
}

static void test2(void)
{
    test_name = "bsearch";
    test_bsearch();
}

static void test3(void)
{
    test_name = "sprintf";
    test_sprintf();
}

static void test4(void)
{
    test_name = "stdio";

    sleep(1);
    servmgr_need("ata");
    servmgr_need("ext2");
    test_stdio();
}

static void test5(void)
{
    test_name = "time";
    test_time();
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

       case 4:
            test4();
            break;

       case 5:
            test5();
            break;

        default:
            printf("* ERROR Unbekannter Testfall\n");
            break;
    }

    quit_qemu();
}
