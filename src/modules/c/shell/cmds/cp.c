/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
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

#include <stdint.h>
#include <dir.h>
#include <io.h>

#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include <lost/config.h>

#define BUFFER_SIZE 4096

void cp_display_usage(void);

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_cp(int argc, char* argv[], const char* args)
#else
    int main(int argc, char* argv[])
#endif
{
    if (argc != 3) {
        cp_display_usage();
        return -1;
    }
    
    char* src_path = argv[1];
    char* dst_path = argv[2];

    FILE* src = fopen(src_path, "r");
    if (src == NULL) {
        fprintf(stderr, "Konnte die Quelldatei nicht oeffnen\n");
        return EXIT_FAILURE;        
    }

    FILE* dst;
    if (is_directory(dst_path)) {
        char* filename = io_split_filename(src_path);
        size_t length = strlen(filename) + strlen(dst_path) + 1;

        char dst_file_path[length + 1];
        strcpy(dst_file_path, dst_path);
        dst_file_path[strlen(dst_file_path) + 1] = '\0';
        dst_file_path[strlen(dst_file_path)] = '/';
        strcat(dst_file_path, filename);
        dst_file_path[length] = '\0';

        dst = fopen(dst_file_path, "w");
    } else {
        dst = fopen(dst_path, "w");
    }

    if (dst == NULL) {
        fprintf(stderr, "Konnte die Zieldatei nicht oeffnen\n");
        fclose(src);
        return EXIT_FAILURE;        
    }

    uint8_t buffer[BUFFER_SIZE];
        
    while (!feof(src)) { 
        size_t length = fread(buffer, BUFFER_SIZE, 1, src);
        if (length == 0) {
            fprintf(stderr, "Fehler beim Kopieren: %d\n", length);
            fclose(src);
            fclose(dst);
            return EXIT_FAILURE;
        }
        
        fwrite(buffer, length, 1, dst);
    }

    fclose(src);
    fclose(dst);

    return EXIT_SUCCESS;
}

void cp_display_usage()
{
    puts("\nAufruf: cp <Quelle> <Ziel>");
}

