/*
 * Copyright (c) 2016 The tyndur Project. All rights reserved.
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
#include <time.h>

struct test_data {
    time_t      time;
    struct tm   tm;
};

static const struct test_data test_data[] = {
    {
        .time = 1451515975,
        .tm = {
            .tm_year = 115,
            .tm_mon  = 11,
            .tm_mday = 30,
            .tm_yday = 363,
            .tm_hour = 22,
            .tm_min  = 52,
            .tm_sec  = 55,
        },
    },
    {
        .time = 1451600229,
        .tm = {
            .tm_year = 115,
            .tm_mon  = 11,
            .tm_mday = 31,
            .tm_yday = 364,
            .tm_hour = 22,
            .tm_min  = 17,
            .tm_sec  =  9,
        },
    },
    {
        .time = 1483222629,
        .tm = {
            .tm_year = 116,
            .tm_mon  = 11,
            .tm_mday = 31,
            .tm_yday = 365,
            .tm_hour = 22,
            .tm_min  = 17,
            .tm_sec  =  9,
        },
    },
    {
        .time = 1456745696,
        .tm = {
            .tm_year = 116,
            .tm_mon  = 1,
            .tm_mday = 29,
            .tm_yday = 59,
            .tm_hour = 11,
            .tm_min  = 34,
            .tm_sec  = 56,
        },
    },
    {
        .time = 951868800,
        .tm = {
            .tm_year = 100,
            .tm_mon  = 2,
            .tm_mday = 1,
            .tm_yday = 60,
            .tm_hour = 0,
            .tm_min  = 0,
            .tm_sec  = 0,
        },
    },
#if 0
    /* TODO Aktivieren, sobald time_t 64 Bit ist */
    {
        .time = -2203891200,
        .tm = {
            .tm_year = 0,
            .tm_mon  = 2,
            .tm_mday = 1,
            .tm_yday = 59,
            .tm_hour = 0,
            .tm_min  = 0,
            .tm_sec  = 0,
        },
    },
    {
        .time = -2206310400,
        .tm = {
            .tm_year = 0,
            .tm_mon  = 1,
            .tm_mday = 1,
            .tm_yday = 59,
            .tm_hour = 0,
            .tm_min  = 0,
            .tm_sec  = 0,
        },
    },
    {
        .time = 4107542400,
        .tm = {
            .tm_year = 200,
            .tm_mon  = 2,
            .tm_mday = 1,
            .tm_yday = 59,
            .tm_hour = 0,
            .tm_min  = 0,
            .tm_sec  = 0,
        },
    },
#endif
};

static bool check_tm(const struct tm* a, const struct tm* b)
{
    if (a->tm_year != b->tm_year)   return false;
    if (a->tm_mon  != b->tm_mon)    return false;
    if (a->tm_mday != b->tm_mday)   return false;
    if (a->tm_yday != b->tm_yday)   return false;
    if (a->tm_hour != b->tm_hour)   return false;
    if (a->tm_min  != b->tm_min)    return false;
    if (a->tm_sec  != b->tm_sec)    return false;

    return true;
}

int test_time(void)
{
    int i;
    int num_tests = sizeof(test_data) / sizeof(test_data[0]);

    for (i = 0; i < num_tests; i++) {
        const struct test_data* t = &test_data[i];
        struct tm tm;

        gmtime_r(&t->time, &tm);

        if (check_tm(&tm, &t->tm)) {
            printf("* PASS %s: Datensatz %d\n", test_name, i);
        } else {
            printf("* FAIL %s: Datensatz %d\n", test_name, i);
            printf("* Ergebnis %d-%d-%d %d:%d:%d; yday %d)\n",
                tm.tm_year, tm.tm_mon, tm.tm_mday,
                tm.tm_hour, tm.tm_min, tm.tm_sec,
                tm.tm_yday);
        }
    }

    return 0;
}
