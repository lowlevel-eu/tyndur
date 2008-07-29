/*
 * Copyright (c) 2008 The LOST Project. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sleep.h>
#include <collections.h>
#include <readline/readline.h>
#include <dir.h>

#include "shell.h"

/**
 * Completion-Handler fuer Readline
 *
 * @param word  Wort, das vervollstaendigt werden soll
 * @param start Position im rl_line_buffer
 * @param end   Endposition im rl_line_buffer
 */
static char** rl_completion(const char* word, int start, int end);

/**
 * Matches in Befehlen suchen
 *
 * @return Pointer auf Array oder NULL wenn keine Gefunden wurden
 */
static char** shell_command_matches(const char* word);

/**
 * Matches in Dateien
 *
 * @return Pointer auf Array oder NULL wenn keine Gefunden wurden
 */
static char** shell_file_matches(const char* word);

/**
 * Baut aus der Liste mit den Matches ein Array
 */
static char** build_matches_array(list_t* list);

/**
 * Vervollstaendigung initialisieren
 */
void completion_init()
{
    rl_attempted_completion_function = rl_completion;
}

static char** rl_completion(const char* word, int start, int end)
{
    char** matches = NULL;

    // Befehle matchen, wenn am Anfang der Zeile
    if (start == 0) {
        matches = shell_command_matches(word);
    }

    if (matches == NULL) {
        matches = shell_file_matches(word);
    }

    return matches;
}

static char** build_matches_array(list_t* list)
{
    char** matches;
    char* match;
    int i;

    matches = malloc(sizeof(char*) * (list_size(list) + 1));
    for (i = 0; (match = list_get_element_at(list, i)); i++) {
        matches[i] = match;
    }
    matches[i] = NULL;

    return matches;
}

static char** shell_command_matches(const char* word)
{
    int word_len = strlen(word);
    shell_command_t* cmd;
    int i;
    char** matches;
    list_t* matches_list = list_create();

    for (i = 0; (cmd = &shell_commands[i]) && cmd->name; i++) {
        if (strncmp(cmd->name, word, word_len) == 0) {
            size_t namelen = strlen(cmd->name);
            char name[namelen + 2];
            strcpy(name, cmd->name);

            // Wir wollen ein schoenes Leerzeichen nach dem Befehl
            name[namelen] = ' ';
            name[namelen + 1] = 0;

            list_push(matches_list, strdup(name));
        }
    }

    if (list_size(matches_list) == 0) {
        list_destroy(matches_list);
        return NULL;
    }

    matches = build_matches_array(matches_list);

    list_destroy(matches_list);

    return matches;
}

static char** shell_file_matches(const char* word)
{
    int word_len = strlen(word);
    char path[word_len + 3];
    char* dirname;
    char* filename;
    int dir_len;
    int filename_len;
    io_resource_t* dir;
    io_direntry_t* dentry;
    char** matches = NULL;
    list_t* matches_list;

    strcpy(path, word);

    // Wenn ein slash am Ende ist, muessen wir noch ein Bisschen was anhaengen,
    // damit dirname den ganzen String nimmt
    if (word[word_len - 1] == '/') {
        dirname = path;
        filename = "";
    } else {
        dirname = io_split_dirname(path);
        filename = io_split_filename(path);
    }
    dir_len = strlen(dirname);
    filename_len = strlen(filename);


    dir = directory_open(dirname);
    if (dir == NULL) {
        goto out;
    }

    matches_list = list_create();
    while ((dentry = directory_read(dir))) {
        size_t namelen = strlen(dentry->name);
        char name[namelen + dir_len + 3];
        char end;

        // Bei Verzeichnissen wollen wir einen Slash am Ende, sonst ein
        // Leerschlag
        if (dentry->type == IO_DIRENTRY_DIR) {
            end = '/';
        } else {
            end = ' ';
        }
        sprintf(name, "%s%s%c", dirname, dentry->name, end);

        if (strncmp(dentry->name, filename, filename_len) == 0) {
            list_push(matches_list, strdup(name));
        }

        free(dentry);
    }

    if (list_size(matches_list) == 0) {
        list_destroy(matches_list);
        goto out;
    }

    matches = build_matches_array(matches_list);

    list_destroy(matches_list);
    directory_close(dir);

out:
    free(dirname);
    free(filename);
    return matches;
}

