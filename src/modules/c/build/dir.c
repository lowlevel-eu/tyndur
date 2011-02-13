/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
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

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "build.h"

#define TMS_MODULE dir
#include <tms.h>

/**
 * Durchsucht ein Verzeichnis und speichert alle fuer das Bauen noetige
 * Informationen in dynamisch allozierten Datenstrukturen, die vom Aufrufer
 * mir free_directory() freigegeben werden muss.
 *
 * Das bedeutet:
 * - Alle Quellcodedateien sammeln (*.c, *.pas, ...)
 * - Includeverzeichnisse speichern
 * - lib-Verzeichnisse erfassen
 * - Bei arch/-Verzeichnissen nur die richtige Architektur beruecksichtigen
 * - .nobuild und .ignorenobuild beachten
 */
struct build_dir* scan_directory(struct build_dir* parent, const char* path)
{
    DIR* dir = opendir(path);
    struct dirent* ent;
    char* suffix;
    struct build_file* file;
    enum filetype filetype;
    struct stat stat_buf;
    struct build_dir* build_dir;
    struct build_dir* subdir;
    char* file_path;
    int i;

    int nobuild = 0;
    int ignorenobuild = 0;

    if (dir == NULL) {
        fprintf(stderr, TMS(not_open, "Verzeichnis %s kann nicht geoeffnet "
            "werden.\n"), path);
        return NULL;
    }

    // Speicher fuer Verzeichnis-struct anlegen und initialiseren
    build_dir = calloc(1, sizeof(*build_dir));
    build_dir->path = strdup(path);
    build_dir->parent = parent;
    build_dir->subdirs = list_create();
    build_dir->obj_files = list_create();
    for (i = 0; i < MAX_LANG; i++) {
        build_dir->src_files[i] = list_create();
    }

    // Dateien und Unterverzeichnisse einlesen
    while ((ent = readdir(dir))) {

        filetype = OTHER_FILE;
        suffix = strrchr(ent->d_name, '.');

        asprintf(&file_path, "%s/%s", path, ent->d_name);
        stat(file_path, &stat_buf);

        // Dateityp erkennen
        if (!strcmp(ent->d_name, ".nobuild")) {
            nobuild = 1;
            goto next;
        } else if (!strcmp(ent->d_name, ".ignorenobuild")) {
            ignorenobuild = 1;
            goto next;
        } else if (*ent->d_name == '.') {
            goto next;
        } else if (S_ISDIR(stat_buf.st_mode)) {
            filetype = SUBDIR;
        } else if (suffix == NULL) {
            filetype = OTHER_FILE;
        } else if (!strcmp(suffix, ".o")) {
            filetype = OBJ;
        } else if (!strcmp(suffix, ".ld")) {
            filetype = LD_SCRIPT;
        } else if (!strcmp(suffix, ".c")) {
            filetype = LANG_C;
        } else if (!strcmp(suffix, ".pas")) {
            filetype = LANG_PAS;
        } else if (!strcmp(suffix, ".asm")) {
            filetype = LANG_ASM_NASM;
        } else if (!strcmp(suffix, ".S")) {
            filetype = LANG_ASM_GAS;
        }

        // Wenn bis hierher nichts verwertbares gefunden wurde, wird die Datei
        // einfach ignoriert.
        if (filetype == OTHER_FILE) {
            goto next;
        }

        // Datei je nach Typ verarbeiten
        file = calloc(1, sizeof(*file));
        file->type = filetype;
        file->name = strdup(ent->d_name);

        switch (filetype) {
            case SUBDIR:
                // Unterverzeichnisse werden je nach Namen unterschiedlich
                // erfasst:
                //
                // - include/ wird zu den Includepfaden hinzugefuegt, das
                //   Verzeicnis wird nicht weiter durchsucht
                // - In include/arch/ und arch/ wird nur das richtige
                //   Unterverzeichnis weiterverfolgt
                // - lib/ wird gesondert gespeichert, damit es vor allen
                //   anderen Verzeichnissen gebaut werden kann
                // - Alles andere wird einfach als Unterverzeichnis gemerkt
                if (!strcmp(file->name, "include")) {
                    DIR* arch_dir;
                    char* tmp;

                    build_dir->has_include = 1;

                    asprintf(&tmp, "include/arch/%s", arch);
                    arch_dir = opendir(tmp);
                    free(tmp);

                    if (arch_dir != NULL) {
                        closedir(arch_dir);
                        build_dir->has_arch_include = 1;
                    }

                } else if (!strcmp(file->name, "lib")) {
                    build_dir->lib = scan_directory(build_dir, file_path);
                } else if (!strcmp(file->name, "arch")) {
                    char* tmp;
                    asprintf(&tmp, "%s/%s", file_path, arch);
                    subdir = scan_directory(build_dir, tmp);
                    free(tmp);
                    if (subdir != NULL) {
                        list_push(build_dir->subdirs, subdir);
                    }
                } else {
                    subdir = scan_directory(build_dir, file_path);
                    if (subdir != NULL) {
                        list_push(build_dir->subdirs, subdir);
                    }
                }
                break;
            case LD_SCRIPT:
                // Ein Linkerskript pro Verzeichnis
                build_dir->ldscript = file->name;
                free(file);
                break;
            case OBJ:
                // Objektdateien haben ihre eigene Liste
                list_push(build_dir->obj_files, file);
                break;
            default:
                // Quellcodedateien werden nach Sprache getrennt gespeichert
                list_push(build_dir->src_files[filetype], file);
                break;
        }

    next:
        free(file_path);
    }

    closedir(dir);

    // Wenn das Verzeichnis gar nicht gebaut werden soll, besser nichts
    // zurueckgeben
    if (nobuild && !ignorenobuild) {
        free_directory(build_dir);
        return NULL;
    }

    return build_dir;
}

/**
 * Gibt die Datenstrukturen fuer ein Verzeichnis wieder frei
 */
void free_directory(struct build_dir* dir)
{
    int i;
    struct build_dir* subdir;
    struct build_file* file;

    if (dir == NULL) {
        return;
    }

    while ((subdir = list_pop(dir->subdirs))) {
        free_directory(subdir);
    }
    list_destroy(dir->subdirs);

    while ((file = list_pop(dir->obj_files))) {
        free(file->name);
        free(file);
    }
    list_destroy(dir->obj_files);

    for (i = 0; i < MAX_LANG; i++) {
        while ((file = list_pop(dir->src_files[i]))) {
            free(file->name);
            free(file);
        }
        list_destroy(dir->src_files[i]);
    }

    free_directory(dir->lib);
    free(dir->path);
    free(dir);
}
