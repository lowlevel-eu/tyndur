/*
 * Copyright (c) 2007-2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann, Alexander Siol
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

#include <types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <lost/config.h>
#include <getopt.h>
#include <errno.h>
#include <lostio.h>

#define BLOCK_SIZE 1024

void bincat_display_usage(void);

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_bincat(int argc, char* argv[], const char* args)
#else
    int main(int argc, char* argv[])
#endif
{
    static const struct option long_options[] =
    {
        { "start", required_argument,  0, 's' },
        { "length", required_argument, 0, 'l' },
        { "end", required_argument,    0, 'e' },
        { "help", no_argument,         0, 'h' },
        { 0, 0, 0, 0 }
    };

    optind = 0;
    uint64_t start = 0;
    uint64_t end = 0;
    uint64_t length = 0;
    char *path = NULL;
    int n = 0;
    int m = 0;
    size_t total_read = 0;


    while (optind < argc) {
        int result = getopt_long(argc, argv, "s:l:e:h", long_options, NULL);
        if (result == -1) {
            break;
        }
        errno = 0;
        switch (result) {
            case 's':
                start = strtoull(optarg, NULL, 0);
                start -= start % 8;
                if (errno) {
                    printf("Ihre Startangabe war ungültig!\n");
                    start = 0;
                }
                break;
            case 'l':
                length = strtoull(optarg, NULL, 0);
                if (errno) {
                    printf("Ihre Längenangabe war ungültig!\n");
                    length = 0;
                }
                break;
            case 'e':
                end = strtoull(optarg, NULL, 0);
                if (errno) {
                    printf("Ihre Endangabe war ungültig!\n");
                    end = 0;
                }
                break;
            case 'h':
                bincat_display_usage();
                return EXIT_SUCCESS;
            default:
                break;
        }
    }

    if (length && end) {
        printf("--length und --end schließen sich aus. Verwende --length\n");
    } else if (end) {
        length = end - start;
    }

    if (optind == argc) {
        bincat_display_usage();
        return EXIT_SUCCESS;
    }

    if (optind + 1 == argc) {
        path = argv[optind];
    } else {
        printf("Fehler: bincat kann nur eine Datei auf einmal verarbeiten!\n");
        return -1;
    }

    FILE* file = fopen(path, "r");

    lio_seek(file, start, SEEK_SET);
    m = start - 8;

    if (file == NULL) {
        printf("Konnte '%s' nicht zum lesen oeffnen!\n", path);
        return -1;
    } else {
        char buffer[BLOCK_SIZE];
        size_t size;
        while (feof(file) != EOF && (total_read <= length || length == 0)) {

            size = fread(buffer, 1, BLOCK_SIZE, file);

            total_read += size;

            if (length && (total_read > length)) {
                size -= total_read - length;
            }

            for (n=0; n < size; n++) {
                if ((n % 8) == 0) {
                    if (n == 8) {
                        printf("0x%08x", m);
                    }
                    m += 8;
                    printf("\n");
                    printf("0x%08x: ", m);
                }

                printf("0x%02x ", ((dword) buffer[n]) & 0xFF);
            }
        }

        fclose(file);
    }
    printf("\n");
    return EXIT_SUCCESS;
}

void bincat_display_usage()
{
    puts("\nAufruf: bincat [Optionen] <Dateiname>\n");
    puts("  -s, --start   Gibt die Startposition an");
    puts("  -l, --length  Gibt die Ausgabelänge an");
    puts("  -e, --end     Gibt die Endposition an\n");
    puts("Falls --length und --end angegeben sind, erhält --length den "
        "Vorrang.\n");
    printf("Position und Längen können optional via vorangestelltem 0x auch "
        "Hexadezimal\n angegeben werden, müssen aber in jedem Fall ganzzahlige "
        "Vielfache der\n Blockgröße (%d Bytes) sein.\n", BLOCK_SIZE);
}

