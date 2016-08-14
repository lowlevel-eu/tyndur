/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
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

#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "dir.h"
#include <lost/config.h>
#include <getopt.h>
#include <collections.h>
#include <string.h>
#include <types.h>
#include <sys/stat.h>
#include <tms.h>

void rm_display_usage(void);
int rm_recursive(char* src_path, bool recurse, bool verbose, int interactive);

enum {
    INTERACTIVE_NEVER,
    INTERACTIVE_ALWAYS,
    INTERACTIVE_ONCE,
};

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_rm(int argc, char* argv[])
#else
    int main(int argc, char* argv[])
#endif
{
    static const struct option long_options[] =
    {
        { "recursive",   no_argument,       0, 'r' },
        { "help",        no_argument,       0, 'h' },
        { "verbose",     no_argument,       0, 'v' },
        { "interactive", optional_argument, 0, 'i' },
        { 0, 0, 0, 0 }
    };

    int result = 0;
    bool verbose = 0;
    bool recursive = 0;
    int directory = 0;
    int interactive = INTERACTIVE_NEVER;
    char* src_path = NULL;

    list_t* sources = list_create();

    optind = 0;
    opterr = 0;

    while (optind < argc) {
        result = getopt_long(argc, argv, "vriIh", long_options, NULL);
        if (result == -1) {
            break;
        }
        switch (result) {
            case 'r':
                recursive = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'i':
                if ((optarg != NULL)) {
                    if (!strcmp("always", optarg)) {
                        interactive = INTERACTIVE_ALWAYS;
                    } else if (!strcmp("once", optarg)) {
                        interactive = INTERACTIVE_ONCE;
                    } else if (!strcmp("never", optarg)) {
                        interactive = INTERACTIVE_NEVER;
                    } else {
                        rm_display_usage();
                        return EXIT_FAILURE;
                    }
                } else {
                    interactive = INTERACTIVE_ALWAYS;
                }
                break;
            case 'I':
                interactive = INTERACTIVE_ONCE;
                break;
            case 'h':
                rm_display_usage();
                return EXIT_SUCCESS;
            case '?':
                printf(TMS(rm_invalid_param, "Unbekannter Parameter: '%c'\n"),
                    optopt);
                return EXIT_FAILURE;
            default:
                break;
        }
    }

    if (optind >= argc) {
        printf(TMS(rm_help, "Probiere 'rm --help' für mehr Informationen.\n"));
        return EXIT_FAILURE;
    }

    while (optind < argc) {
        char* p = strdup(argv[optind++]);
        int len = strlen(p);
        if (p[len - 1] == '/') {
            p[len - 1] = 0;
        }
        list_push(sources, p);
    }

    while ((src_path = list_pop(sources))) {

        directory = is_directory(src_path);
        if (directory) {
            if (!recursive) {
                printf(TMS(rm_isdir, "'%s/' ist ein Verzeichnis:"
                    " Übersprungen\n"),
                    src_path);
                result = -1;
            } else {
                result += rm_recursive(src_path, true, verbose, interactive);
            }
        } else {
            result += rm_recursive(src_path, false, verbose, interactive);
        }
    }
    list_destroy(sources);


    if (result != 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void rm_display_usage()
{
    printf(TMS(rm_usage,
        "\nAufruf: rm [Optionen] <Pfad>\n\n"
        "Optionen:\n"
        "  -r, --recursive    Verzeichnis rekursiv löschen\n"
        "  -v, --verbose      Statusmeldungen anzeigen\n"
        "  -i, --interactive  <once|always|never>\n"
        "                     once: Einmal fragen, ob <Pfad>"
        " gelöscht werden soll\n"
        "                     always: Immer fragen, ob <Pfad>"
        " gelöscht werden soll\n"
        "                     never: Nie fragen, ob <Pfad>"
        " gelöscht werden soll\n"
        "  -h, --help         diese Hilfe anzeigen\n\n"
        "Anstatt -i once kann auch -I verwendet werden.\n"));
}

int rm_recursive(char* src_path, bool recurse, bool verbose, int interactive)
{
    int result = 0;
    char* path = NULL;
    struct dir_handle* dir_res;
    char c;
    struct stat buf;
    if ((stat(src_path, &buf))) {
        printf(TMS(rm_doesnt_exist, "'%s' ist inexistent!\n"), src_path);
        return -1;
    }


    switch (interactive) {
        case INTERACTIVE_ALWAYS:
            printf(TMS(rm_question_del, "'%s' löschen?\n"), src_path);
            fflush(stdout);
            while ((c = getchar()) == EOF);
            if (c != 'y') {
                printf(TMS(rm_message_skip, "'%s' übersprungen.\n"), src_path);
                return -1;      // Fehlschlag
            }
            break;
        case INTERACTIVE_ONCE:
            printf(TMS(rm_question_del, "'%s' löschen?\n"), src_path);
            fflush(stdout);
            while ((c = getchar()) == EOF);
            if (c != 'y') {
                printf(TMS(rm_message_skip, "'%s' übersprungen.\n"), src_path);
                return -1;
            }
            interactive = INTERACTIVE_NEVER;
            break;
        default:
            break;
    }
    if (verbose) {
        printf(TMS(rm_message_del, "Lösche '%s'.\n"), src_path);
    }
    if (recurse && (dir_res = directory_open(src_path))) {
        io_direntry_t* direntry;
        list_t* fn_list = list_create();

        while ((direntry = directory_read(dir_res))) {
            if (!strcmp(direntry->name, ".") ||
                !strcmp(direntry->name, ".."))
            {
                continue;
            }
            asprintf(&path, "%s/%s", src_path, direntry->name);
            list_push(fn_list, path);
            free(direntry);
        }
        directory_close(dir_res);

        while ((path = list_pop(fn_list))) {
            rm_recursive(path, recurse, verbose, interactive);
            free(path);
        }
        list_destroy(fn_list);
        io_remove_link(src_path);
    } else {
        result = io_remove_link(src_path);
        if (result != 0) {
            printf(TMS(rm_error, "'%s' konnte nicht gelöscht werden!\n"), src_path);
        }
    }
    return result;
}

