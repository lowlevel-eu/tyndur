/*
 * Copyright (c) 2007-2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf, Alexander Siol
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

#include <stdint.h>
#include <dir.h>
#include <io.h>

#include <types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <collections.h>
#include <lost/config.h>
#include <tms.h>

#define BUFFER_SIZE 4096

static int verbosity = 0;

void cp_display_usage(void);
int cp_file(char* src_path, char* dst_path);
int cp_recursive(char* src_path, char* dst_path);

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_cp(int argc, char* argv[])
#else
    int main(int argc, char* argv[])
#endif
{
    static const struct option long_options[] =
    {
        { "target", required_argument, 0, 't' },
        { "recursive", no_argument,    0, 'r' },
        { "verbose", no_argument,      0, 'v' },
        { "help", no_argument,         0, 'h' },
        { 0, 0, 0, 0 }
    };

    optind = 0;
    verbosity = 0;

    char* targetpath = NULL;
    list_t* sources = list_create();

    int index = -1;
    int result;

    int recursive = 0;

    while (optind < argc) {
        result = getopt_long(argc, argv, "vrt:h", long_options, &index);
        if (result == -1) {
            break;
        }
        switch (result) {
            case 't':
                targetpath = optarg;
                break;
            case 'r':
                recursive = 1;
                break;
            case 'v':
                verbosity++;
                break;
            case 'h':
                cp_display_usage();
                return EXIT_SUCCESS;
            default:
                break;
        }
    }

    while (optind < argc) {
        list_push(sources, argv[optind++]);
    }

    // Falls kein Target-Parameter gegeben ist, letztes Element des Listings.
    if (targetpath == NULL) {
        targetpath = (char*)list_pop(sources);
    }

    if (targetpath == NULL || list_size(sources) == 0) {
        cp_display_usage();
        goto bad_exit;
    }

    if ( (list_size(sources) > 1) && (!is_directory(targetpath)) ) {
        fprintf(stderr, TMS(cp_error_dst_no_dir,
            "Fehler: Mehrere Quellen, aber Ziel ist kein"
            " Verzeichnis!\n"));
        goto bad_exit;
    }

    char* src_path = NULL;
    while ( (src_path = (char*)list_pop(sources)) ) {
        if (is_directory(src_path)) {
            if (recursive) {
                cp_recursive(src_path, targetpath);
            } else {
                printf(TMS(cp_skip,
                    "'%s' übersprungen (Meinten Sie 'cp -r'?)\n"),
                    src_path);
                goto bad_exit;
            }
        } else {
            result = cp_file(src_path, targetpath);
            if (result == -1) {
                printf(TMS(cp_src_opening_error,
                    "Konnte die Quelldatei nicht öffnen!\n"
                    "Pfad: '%s'\n"), src_path);
                goto bad_exit;
            }  else if (result == -2) {
                printf(TMS(cp_dst_opening_error,
                    "Konnte die Zieldatei nicht öffnen!\n"
                    "Pfad: '%s'\n"), targetpath);
                goto bad_exit;
            }
        }
    }

    list_destroy(sources);
    return EXIT_SUCCESS;

    bad_exit:
    list_destroy(sources);
    return EXIT_FAILURE;
}

void cp_display_usage()
{
    printf(TMS(cp_usage,
        "\nAufruf: cp [Optionen] <Quelle> <Ziel>\n"
        "  oder: cp [Optionen] <Quelle> ... <Verzeichnis>\n"
        "  oder: cp [Optionen] -t <Verzeichnis> <Quelle> ...\n\n"
        "Optionen:\n"
        "  -v, --verbose    durchgeführte Tätigkeiten erklären\n"
        "  -r, --recursive  Verzeichnisse rekursiv kopieren\n"
        "  -h, --help       diese Hilfe anzeigen\n"));
}

int cp_recursive(char* src_path, char* dst_path)
{
    int length = -1;
    if (is_directory(dst_path)) {
        length = asprintf(&dst_path, "%s/%s", dst_path, io_split_filename(src_path));
        if (length < 0) {
            // src_path hilft hier nicht, dst_path ist per asprintf undefiniert, 
            // sollte also nicht verwendet werden. Wahrscheinlichste Fehlerursache
            // bleibt aber OOM, ergo sind beide Pfade nicht hilfreich.
            printf(TMS(cp_error_dst_path,
                "Fehler beim Setzen des Zielpfades!\n"));
            return -1;
        }
    } 
    if (!directory_create(dst_path)) {
        fprintf(stderr, TMS(cp_error_create_dst,
            "Fehler: Pfad '%s' konnte nicht angelegt werden!\n"
            "        '%s' übersprungen.\n"), dst_path, src_path);
        if (length >= 0) {
            free(dst_path);
        }
        return -1;
    }
    if (verbosity > 0) {
        printf("'%s' -> '%s'\n", src_path, dst_path);
    }
    
    struct dir_handle* dir_res = directory_open(src_path);
    char* full_src_path = NULL;
    char* full_dst_path = NULL;
    if (dir_res != NULL) {
        io_direntry_t* direntry;
        while ((direntry = directory_read(dir_res))) {
            if (strcmp(direntry->name, ".") && strcmp(direntry->name, "..")) {
                asprintf(&full_src_path, "%s/%s", src_path, direntry->name);
                asprintf(&full_dst_path, "%s/%s", dst_path, direntry->name);
                if (direntry->type == IO_DIRENTRY_FILE) {
                    cp_file(full_src_path, full_dst_path);
                } else {
                    cp_recursive(full_src_path, full_dst_path);
                }
                free(full_src_path);
                free(full_dst_path);
            }
            free(direntry);
        }
        directory_close(dir_res);
        if (length >= 0) {
            free(dst_path);
        }
        return 1;
    } else {
        if (length >= 0) {
            free(dst_path);
        }
        return -1;
    }
}

int cp_file(char* src_path, char* dst_path) {
    FILE* src = fopen(src_path, "r");
    if (src == NULL) {
        return -1;
    }

    int free_dst_path = 0;
    FILE* dst = NULL;
    if (is_directory(dst_path)) {
        char* filename = io_split_filename(src_path);
        asprintf(&dst_path, "%s/%s", dst_path, filename);
        free(filename);
        free_dst_path = 1;
    }
    dst = fopen(dst_path, "w");

    if (verbosity > 0) {
        printf("'%s' -> '%s'\n", src_path, dst_path);
    }

    if (free_dst_path) {
        free(dst_path);
    }

    if (dst == NULL) {
        return -2;
    }

    uint8_t buffer[BUFFER_SIZE];

    while (!feof(src)) {
        size_t length = fread(buffer, 1, BUFFER_SIZE, src);
        if (ferror(src)) {
            printf(TMS(cp_error_src_read,
                "Beim Lesen der Quelldatei ist ein Fehler"
                " aufgetreten!\n"));
            fclose(src);
            fclose(dst);
            return -3;
        }

        fwrite(buffer, length, 1, dst);
    }

    fclose(src);
    fclose(dst);

    return 1;
}
