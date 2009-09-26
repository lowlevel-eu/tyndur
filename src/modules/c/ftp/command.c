/**
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Paul Lange.
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
#include <stdlib.h>
#include <types.h>
#include <ctype.h>
#include <libgen.h>
#include <sys/stat.h>
#include <readline/readline.h>

#include "ftp.h"

#define BLOCK_128K 131072

void ftp_account(void)
{
    char* input, *prompt, *c;

    // falls keine Verbindung besteht, abbrechen
    if (handle == NULL) {
        puts("\033[1;37mes besteht keine Serververbindung...\033[0m");
        return;
    }

    asprintf(&prompt, "\033[1;37mBenutzer (%s): \033[0m", host);
    input = readline(prompt);
    free(prompt);

    // sorgt dafür, dass die Leerzeichen entfernt werden
    for (; isspace(*input); input++);
    for (c = input; !isspace(*c) && *c; c++);
    *c = '\0';

    fprintf(handle, "USER %s\r\n", input);
    fflush(handle);
    response();

    free(input);
    input = readline("\033[1;37mPasswort: \033[0m");

    for (; isspace(*input); input++);
    for (c = input; !isspace(*c) && *c; c++);
    *c = '\0';

    fprintf(handle, "PASS %s\r\n", input);
    fflush(handle);
    response();
    free(input);
}

void ftp_ls(void)
{
    FILE* data_handle;
    char buffer[BLOCK_128K];

    if (handle == NULL) {
        puts("\033[1;37mes besteht keine Serververbindung...\033[0m");
        return;
    }

    data_handle = ftp_data_connect();
    request("LIST");

    while (!feof(data_handle)) {
      fwrite(buffer, 1, fread(buffer, 1, BLOCK_128K, data_handle),stdout);
    }

    // das verbraucht noch viel Zeit, da tyndur lange braucht für das fclose()
    fclose(data_handle);
    response();
    puts("\033[1;37mDatenverbindung beendet\033[0m");
}

void ftp_get(char* pathname)
{
    FILE* data_handle;
    FILE* file_handle;
    char* filename;
    char buffer[BLOCK_128K];

    if (handle == NULL) {
        puts("\033[1;37mes besteht keine Serververbindung...\033[0m");
        return;
    }

    filename = basename(pathname);
    file_handle = fopen(filename, "w+");
    data_handle = ftp_data_connect();

    fprintf(handle, "RETR %s\r\n", pathname);
    fflush(handle);

    // falls 550 zurückgegeben wird, ist die Datei nicht vorhanden
    response();

    while (!feof(data_handle)) {
      fwrite(buffer, 1, fread(buffer, 1, BLOCK_128K, data_handle),file_handle);
    }

    fclose(file_handle);
    fclose(data_handle);
    response();
    puts("\033[1;37mDatenverbindung beendet\033[0m");
}

void ftp_put(char* pathname)
{
    FILE* data_handle;
    FILE* file_handle;
    char* filename;
    char buffer[BLOCK_128K];
    size_t count;

    if (handle == NULL) {
        puts("\033[1;37mes besteht keine Serververbindung...\033[0m");
        return;
    }

    filename = basename(pathname);
    file_handle = fopen(pathname, "r");
    data_handle = ftp_data_connect();

    fprintf(handle, "STOR %s\r\n", filename);
    fflush(handle);
    response();

    while (!feof(file_handle)) {
        count = fread(buffer, 1, BLOCK_128K, file_handle);
        fwrite(buffer, 1, count, data_handle);
    }

    fclose(file_handle);
    fclose(data_handle);
    response();
    puts("\033[1;37mDatenverbindung beendet\033[0m");
}

void ftp_mkdir(const char* pathname)
{
    if (handle == NULL) {
        puts("\033[1;37mes besteht keine Serververbindung...\033[0m");
        return;
    }

    fprintf(handle, "MKD %s\r\n", pathname);
    fflush(handle);
    response();
}

void ftp_rmdir(const char* pathname)
{
    if (handle == NULL) {
        puts("\033[1;37mes besteht keine Serververbindung...\033[0m");
        return;
    }

    fprintf(handle, "RMD %s\r\n", pathname);
    fflush(handle);
    response();
}

void ftp_cd(const char* pathname)
{
    if (handle == NULL) {
        puts("\033[1;37mes besteht keine Serververbindung...\033[0m");
        return;
    }

    fprintf(handle, "CWD %s\r\n", pathname);
    fflush(handle);
    response();
}

void ftp_cdup(void)
{
    if (handle == NULL) {
        puts("\033[1;37mes besteht keine Serververbindung...\033[0m");
        return;
    }

    fprintf(handle, "%s\r\n", "CDUP");
    fflush(handle);
    response();
}
