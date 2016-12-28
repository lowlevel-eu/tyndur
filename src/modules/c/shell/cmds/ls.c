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

#include <stdbool.h>
#include "stdlib.h"
#include "stdio.h"
#include "dir.h"
#include "unistd.h"
#include <lost/config.h>
#include <getopt.h>
#include <collections.h>
#include <lostio.h>
#include <tms.h>


#define LS_COLOR_RESET_DEFAULT  "00"
#define LS_COLOR_TYPE           "32"
#define LS_COLOR_FILE_DEFAULT   "00"
#define LS_COLOR_DIR_DEFAULT    "01;34"
#define LS_COLOR_LINK_DEFAULT   "01;36"

void ls_display_usage(void);
char* format_size(size_t size, bool human_readable);

enum {
    OPT_NO_COLOR = 256,
};

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_ls(int argc, char* argv[])
#else
    int main(int argc, char* argv[])
#endif
{
    char* dir_path;
    char* cwd_dir = NULL;
    bool success = true;
    bool show_hidden = false;
    bool human_readable = false;
    bool colorize = false;
    list_t *dirs = list_create();
    bool print_dirnames = false;

    static const struct option long_options[] =
    {
        { "all", no_argument,            0, 'a' },
        { "help", no_argument,           0,   1 },
        { "human-readable", no_argument, 0, 'h' },
        { "color", no_argument,          0, 'C' },
        { "no-color", no_argument,       0, OPT_NO_COLOR },
        { 0, 0, 0, 0 }
    };

    optind = 0;

    if (file_is_terminal(stdout)) {
        colorize = true;
    }

    while (optind < argc) {
        int result = getopt_long(argc, argv, "ahC", long_options, NULL);
        if (result == -1) {
            break;
        }
        switch (result) {
            case 'a':
                show_hidden = true;
                break;
            case 'h':
                human_readable = true;
                break;
            case 'C':
                colorize = true;
                break;
            case OPT_NO_COLOR:
                colorize = false;
                break;
            case 1:
                ls_display_usage();
                return EXIT_SUCCESS;
            default:
                break;
        }
    }

    while (optind < argc) {
        list_push(dirs, argv[optind++]);
    }

    if (list_size(dirs) == 0) {
        cwd_dir = getcwd(NULL, 0);
        list_push(dirs, cwd_dir);
    }

    if (list_size(dirs) > 1) {
        print_dirnames = true;
    }

    while ((dir_path = list_pop(dirs))) {
        if (print_dirnames) {
            printf("%s:\n", dir_path);
        }
        struct dir_handle* dir_res = directory_open(dir_path);
        if (dir_res != NULL) {
            io_direntry_t* direntry;
            while ((direntry = directory_read(dir_res))) {
                bool entry_hidden = (*direntry->name == '.');
                if (!entry_hidden || show_hidden) {
                    char *formatted_size = format_size(direntry->size,
                            human_readable);
                    char flags[4] = "   ";
                    const char *color = NULL;

                    if (direntry->type & IO_DIRENTRY_FILE) {
                        flags[0] = 'f';
                        color = LS_COLOR_FILE_DEFAULT;
                    }
                    if (direntry->type & IO_DIRENTRY_DIR) {
                        flags[1] = 'd';
                        color = LS_COLOR_DIR_DEFAULT;
                    }
                    if (direntry->type & IO_DIRENTRY_LINK) {
                        flags[2] = 'l';
                        color = LS_COLOR_LINK_DEFAULT;
                    }

                    if (colorize) {
                        printf(" [\e[%sm%s\e[%sm]   %s \e[%sm%s\e[%sm\n",
                               LS_COLOR_TYPE, flags, LS_COLOR_RESET_DEFAULT,
                               formatted_size,
                               color, direntry->name, LS_COLOR_RESET_DEFAULT);
                    } else {
                        printf(" [%s]   %s %s\n", flags, formatted_size,
                            direntry->name);
                    }
                    free(formatted_size);
                }
                free(direntry);
            }

            directory_close(dir_res);
        } else {
            printf(TMS(ls_opening_error,
                "Konnte '%s' nicht zum lesen öffnen!\n"), dir_path);
            success = false;
        }
        if (list_size(dirs) > 0) {
            printf("\n");
        }
    }

    free(cwd_dir);


    list_destroy(dirs);

    if (success == true) {
        return EXIT_SUCCESS;
    } else {
        return -1;
    }
}

char* format_size(size_t size, bool human_readable)
{
    char* retval = NULL;
    if (!human_readable) {
        asprintf(&retval, "% 9d", size);
    } else {
        int unit = 0;
        char units[] = " KMGT";
        while (size > 1024 && units[unit]) {
            size /= 1024;
            unit++;
        }
        if (unit == 0) {
            asprintf(&retval, "% 6d", size);
        } else {
            asprintf(&retval, " % 4d%c", size, units[unit]);
        }
    }
    return retval;
}

void ls_display_usage()
{
    printf(TMS(ls_usage,
        "\nAufruf: ls [Optionen] [Verzeichnisse]\n\n"
        "Optionen:\n"
        "  -a, --all             zeige versteckte Dateien an\n"
        "  -C, --color           zeige Ausgabe farbig\n"
        "  -h, --human-readable  zeige Dateigröße menschenlesbar an "
        "(z.B. 10M, 1G)\n"
        "      --help            diese Hilfe anzeigen\n"));
}

