/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
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
#include <lost/config.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <tms.h>

void stat_display_usage(void);
void stat_display_results(const char* path, struct stat* stat_buf);

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_stat(int argc, char* argv[])
#else
    int main(int argc, char* argv[])
#endif
{
    const char* path;
    struct stat stat_buf;

    if (argc != 2) {
        stat_display_usage();
        return EXIT_FAILURE;
    }
    
    path = argv[1];
    if (stat(path, &stat_buf) != 0) {
        fprintf(stderr, TMS(stat_error, "Fehler: Datei nicht gefunden!\n"));
        return EXIT_FAILURE;
    }
    stat_display_results(path, &stat_buf);
    return EXIT_SUCCESS;
}

void stat_display_usage()
{
    printf(TMS(stat_usage, "\nAufruf: stat <Pfad>\n"));
}

void stat_display_results(const char* path, struct stat* stat_buf)
{
    printf(TMS(stat_message_no1, "   Datei: '%s'\n"), path);

    printf(TMS(stat_message_no2,
        " Groesse: %8d  Blocks: %8d  Blockgroesse: %8d\n"),
        stat_buf->st_size,
        stat_buf->st_blocks,
        stat_buf->st_blksize);

    printf(TMS(stat_message_no3, "     Typ: "));

    if (S_ISDIR(stat_buf->st_mode)) {
        printf(TMS(stat_message_no4, "Verzeichnis"));
    } else {
        printf(TMS(stat_message_no5, "Regulaere Datei"));
    }

    printf("\n");

}

