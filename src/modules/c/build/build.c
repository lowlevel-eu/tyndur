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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include "build.h"

#define TMS_MODULE build
#include <tms.h>

#ifdef __LINUX__
static const char* cflags = "-m32 -g -O0 -Wall -fno-stack-protector -nostdinc"
    " -fno-leading-underscore -fno-omit-frame-pointer -fno-strict-aliasing"
    " -fno-builtin -c";
static const char* fpcflags = "-n -Cn -CX -Ttyndur";
#else
//static const char* cflags = "-c -combine";
static const char* cflags = "-c";
static const char* fpcflags = "-Cn -CX -Ttyndur";
#endif

static const char* nasmflags = "-felf";
static const char* gasflags = "-m32 -c";

static const char* c_compiler = "gcc";
static const char* linker = "gcc";

static char* root_path;

/**
 * Fuehrt einen Shellbefehl in einem gegebenen Arbeitsverzeichnis aus
 */
static void do_command(const char* path, const char* binary,
    const char* args_fmt, ...)
{
    char* buf;
    char* buf2;
    char* dirname;
    va_list ap;

    asprintf(&dirname, "%s/%s", root_path, path);
    if (verbose) {
        printf(">>> cd %s\n", dirname);
    }
    chdir(dirname);
    free(dirname);

    va_start(ap, args_fmt);
    vasprintf(&buf, args_fmt, ap);
    va_end(ap);

    asprintf(&buf2, "%s %s", binary, buf);

    if (verbose) {
        printf(">>> %s\n", buf2);
    }
    if (!dry_run) {
        system(buf2);
    }

    free(buf);
    free(buf2);
    chdir(root_path);
}

/**
 * Sammelt alle Objektdateien in einem gegebenen Pfad ein und gibt sie durch
 * Leerzeichen getrennt zurueck.
 *
 * FIXME Mit Leerzeichen im Dateinamen ist das offensichtlich kaputt
 */
static char* get_obj_files(const char* path)
{
    struct build_dir* dir;
    struct build_dir* subdir;
    struct build_file* file;
    char* tmp = NULL;
    char* subdir_files;
    char* file_list = strdup("");
    int i;

    dir = scan_directory(NULL, path);
    for (i = 0; (file = list_get_element_at(dir->obj_files, i)); i++) {
        tmp = file_list;
        asprintf(&file_list, "%s %s/%s", tmp, dir->path, file->name);
        free(tmp);
    }

    for (i = 0; (subdir = list_get_element_at(dir->subdirs, i)); i++) {
        tmp = file_list;
        subdir_files = get_obj_files(subdir->path);
        asprintf(&file_list, "%s %s", tmp, subdir_files);
        free(subdir_files);
        free(tmp);
    }

    free_directory(dir);

    return file_list;
}

/**
 * Kompiliert eine Anzahl von Dateien
 *
 * @param path Arbeitsverzeichnis
 * @param files Liste von zu kompilierenden Dateinamen
 * @param compiler Dateiname des Compilers
 * @param flags Flags, die in den Aufruf eingebunden werden sollen
 * @param include Zusaezlicher String, der in den Aufruf eingebunden werden
 * soll; primaer fuer die Angabe von Includeverzeichnissen.
 */
static void compile(const char* path, list_t* files, const char* compiler,
    const char* flags, const char* include)
{
    struct build_file* file;
    char* file_list = strdup("");
    char* tmp;
    int i;

    for (i = 0; (file = list_get_element_at(files, i)); i++) {
        tmp = file_list;
        asprintf(&file_list, "%s %s", tmp, file->name);
        free(tmp);
    }

    if (*file_list) {
        do_command(path, compiler, "%s -I %s/%s %s %s",
            flags, root_path, path, include, file_list);
    }
}

/**
 * Baut ein Projektverzeichnis
 *
 * @param dir Verzeichnis, das gebaut werden soll
 * @param parent_include Includeverzeichnisse des Vaterverzeichnisses, die fuer
 * Quellcodedateien in diesem Verzeichnis zur Verfuegung stehen sollen
 * @param parent_lib lib-Verzeichnisse der Vaterverzeichnisse, deren
 * Bibliotheken in diesem Verzeichnis zur Verfuegung stehen sollen
 * @param depth Verschachtelungstiefe ausgehend vom Wurzelverzeichnis, in dem
 * gebaut wird (fuer Baumdarstellung)
 */
static void do_build(struct build_dir* dir, const char* parent_include,
    const char* parent_lib, int depth)
{
    char* include;
    char* lib;
    char* tmp = NULL;

    char* objs;
    struct build_dir* subdir;
    struct build_file* file;
    int i;

    for (i = 0; i < depth; i++) {
        printf("| ");
    }
    printf("+ %s\n", dir->path);

    // Ggf. include-Verzeichnis anhaengen
    if (dir->has_include) {
        asprintf(&include, "%s -I %s/%s/include",
            parent_include, root_path, dir->path);
    } else {
        include = strdup(parent_include);
    }

    if (dir->has_arch_include) {
        tmp = include;
        asprintf(&include, "%s -I %s/%s/include/arch/%s",
            tmp, root_path, dir->path, arch);
        free(tmp);
    }

    // Ggf. Lib-Verzeichnis anhaengen
    if (dir->lib != NULL) {
        asprintf(&lib, "%s %s/%s/lib/library.a",
            parent_lib, root_path, dir->path);
    } else {
        lib = strdup(parent_lib);
    }

    // lib-Verzeichnis wird als erstes kompiliert
    if (dir->lib) {
        do_build(dir->lib, include, lib, depth + 1);
        objs = get_obj_files(dir->lib->path);
        do_command(".", "ar", "rs %s/library.a %s",
            dir->lib->path, objs);
        free(objs);
    }

    // Dann alle anderen Unterverzeichnisse
    for (i = 0; (subdir = list_get_element_at(dir->subdirs, i)); i++) {
        do_build(subdir, include, lib, depth + 1);
    }

    // Und schliesslich noch die einzelnen Dateien
    // TODO Pruefen, ob sich Abhaengigkeiten veraendert haben
    if (!dont_compile) {
        printf(TMS(compile_c, "%s: Kompilieren (C)..."), dir->path);
        fflush(stdout);
        compile(dir->path, dir->src_files[LANG_C], c_compiler, cflags, include);

        printf(TMS(compile_pascal, "\r%s: Kompilieren (Pascal)...\033[K"),
            dir->path);
        fflush(stdout);
        compile(dir->path, dir->src_files[LANG_PAS], "fpc", fpcflags, include);

        printf(TMS(assemble_gas, "\r%s: Assemblieren (gas)...\033[K"),
            dir->path);
        fflush(stdout);
        compile(dir->path, dir->src_files[LANG_ASM_GAS], "gcc", gasflags, include);

        printf(TMS(assemble_yasm, "\r%s: Assemblieren (yasm)...\033[K"),
            dir->path);
        fflush(stdout);
        for (i = 0; (file = list_get_element_at(dir->src_files[LANG_ASM_NASM], i)); i++) {
            do_command(dir->path, "yasm", "%s %s", nasmflags, file->name);
        }
    }

    // Im Wurzelverzeichnis wird gelinkt (TODO einstellbar machen)
    if (dir->parent == NULL) {
        printf(TMS(link, "\r%s: Linken...\033[K"), dir->path);
        fflush(stdout);

        objs = get_obj_files(dir->path);
        if (!standalone) {
            do_command(".", linker, "-o %s/run %s %s", dir->path, lib, objs);
        } else {
            do_command(".", linker, "-o %s/run %s%s "
                "--start-group %s %s --end-group",
                dir->path,
                dir->ldscript ? "-T" : "",
                dir->ldscript ? dir->ldscript : "",
                lib, objs);
        }
        free(objs);
    }

    printf("\r\033[K");
    fflush(stdout);

    free(include);
    free(lib);
}

/**
 * Bauen!
 *
 * @param root Wurzelverzeichnis des Projekts
 * @param standalone Wenn != 0, wird die Standardbibliothek nicht benutzt
 */
void build(struct build_dir* root)
{
    if (standalone) {
        c_compiler = "gcc -nostdinc";
        linker = "ld";
    }
    root_path = getcwd(NULL, 0);
    do_build(root, "", "", 0);
    printf(TMS(done, "Fertig gebaut.\n"));
    free(root_path);
}
