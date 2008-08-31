/*
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
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
#include "dir.h"
#include "unistd.h"
#include <lost/config.h>

void ls_display_usage(void);

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_ls(int argc, char* argv[], const char* args)
#else
    #define _USE_START_
    #include "init.h"
    int main(int argc, char* argv[])
#endif
{
    if (argc > 2) {
        ls_display_usage();
        return -1;
    }
    
    char* dir_path;
    bool free_dir_path;
    bool success = TRUE;

    if (argc == 1) {
        //Mit NULL als Parameter wird der buffer mit malloc() erstellt
        dir_path = getcwd(NULL, 0);
        free_dir_path = TRUE;
    } else {
        dir_path = argv[1];
        free_dir_path = FALSE;
    }


    io_resource_t* dir_res = directory_open(dir_path);
    if (dir_res != NULL) {
        io_direntry_t* direntry;
        while ((direntry = directory_read(dir_res))) {
            if (direntry->type == IO_DIRENTRY_FILE) {
                printf("       %s\n", direntry->name);
            } else {
                printf(" [DIR] %s\n", direntry->name);
            }
           free(direntry);
        }

        directory_close(dir_res);
    } else {
        printf("Konnte '%s' nicht zum lesen oeffnen!\n", dir_path);
        success = FALSE;
    }

    if (free_dir_path == TRUE) {
        free(dir_path);
    }
    
    if (success == TRUE) {
        return EXIT_SUCCESS;
    } else {
        return -1;
    }
}

void ls_display_usage()
{
    puts("\nAufruf: ls [Verzeichnis]\n");
}

