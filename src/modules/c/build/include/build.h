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

#ifndef _BUILD_H_
#define _BUILD_H_

#include <collections.h>

enum filetype {
    LANG_C,
    LANG_PAS,
    LANG_ASM_GAS,
    LANG_ASM_NASM,

#define MAX_LANG OTHER_FILE
    OTHER_FILE,
    LD_SCRIPT,
    SUBDIR,
    OBJ,
};


extern char* filetype_str[];
extern char* arch;

struct build_dir {
    int                 has_include;
    int                 has_arch_include;
    char*               path;
    char*               ldscript;
    struct build_dir*   parent;
    struct build_dir*   lib;
    list_t*             src_files[MAX_LANG];
    list_t*             obj_files;
    list_t*             subdirs;
};

struct build_file {
    enum filetype   type;
    char*           name;
};

struct build_dir* scan_directory(struct build_dir* parent, const char* path);
void free_directory(struct build_dir* dir);
void build(struct build_dir* root);

extern int dry_run;
extern int verbose;
extern int standalone;
extern int dont_compile;

#endif
