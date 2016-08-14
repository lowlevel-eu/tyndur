/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <syscall.h>
#include <lost/config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tms.h>
#include <getopt.h>

#define MAX_BLOCK_SIZE 524288

static void usage(void)
{
    printf(TMS(bench_usage, "\nAufruf: bench [-s] <Dateiname> <Kilobytes>\n"));
}

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_bench(int argc, char* argv[])
#else
    int main(int argc, char* argv[])
#endif
{
    char buffer[MAX_BLOCK_SIZE];
    size_t count, n, size, rsize;
    size_t bs_large[] = { 524288, 65536, 4096, 512, 0 };
    size_t bs_small[] = { 16, 1, 0 };
    size_t* bs;
    int i;
    char* path;
    FILE* file;
    uint64_t tick_start, tick_end, diff;
    bool small_reads = false;

    optind = 1;
    while (optind < argc) {
        int result = getopt(argc, argv, "s");
        if (result == -1) {
            break;
        }

        switch (result) {
            case 's':
                small_reads = true;
                break;
            default:
                usage();
                return EXIT_FAILURE;
        }
    }

    if (optind != argc - 2) {
        usage();
        return EXIT_FAILURE;
    }

    path = argv[optind++];
    count = atoi(argv[optind++]);

    bs = small_reads ? bs_small : bs_large;

    for (i = 0; bs[i]; i++)
    {
        if (small_reads) {
            n = count * 1024;
        } else {
            n = ((count + 63) & ~63) * 1024;
        }

        puts("");

        file = fopen(path, "r");
        if (file == NULL) {
            printf(TMS(bench_opening_error,
                "Konnte '%s' nicht zum lesen öffnen!\n"), path);
            return EXIT_FAILURE;
        }

        tick_start = get_tick_count();
        while (n) {
            size = bs[i] > n ? n : bs[i];
            rsize = fread(buffer, 1, size, file);
            if (size != rsize) {
                printf(TMS(bench_reading_error,
                    "Beim Lesen ist ein Fehler aufgetreten!\n"));
                break;
            }
            n -= rsize;

            if (n % 65536 == 0) {
                tick_end = get_tick_count();
                diff = (tick_end - tick_start);
                if (diff == 0) {
                    printf(TMS(bench_message_no1,
                        "\rBlockgroesse: %6d; %5d/%5d kB uebrig;"
                        " Rate: -- Byte/s [%lld s] "),
                        bs[i], n / 1024, count,
                        diff / 1000000);
                } else {
                    printf(TMS(bench_message_no2,
                        "\rBlockgroesse: %6d; %5d/%5d kB uebrig;"
                        " Rate: %10lld Byte/s [%lld s] "),
                        bs[i], n / 1024, count,
                        ((uint64_t) count * 1024 - n) * 1000000 / diff,
                        diff / 1000000);
                }
            }
        }

        fclose(file);
    }
    puts("");

    return EXIT_SUCCESS;
}
