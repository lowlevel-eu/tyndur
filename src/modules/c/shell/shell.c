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

#include <stdint.h>
#include <stdbool.h>
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "rpc.h"
#include "io.h"
#include "dir.h"
#include "lostio.h"
#include <lost/config.h>
#include <sleep.h>
#include <env.h>

#define TMS_MODULE shell
#include <tms.h>

#include <readline/readline.h>
#include <readline/history.h>
#include <init.h>


#include "shell.h"

#define COMMAND_BUFFER_SIZE 255




char    shell_command_buffer[COMMAND_BUFFER_SIZE];

char    keyboard_read_char(void);
void    shell_read_command(void);
int     shell_do_cmd(void);


shell_command_t shell_commands[] = {
    {"version",     &shell_command_version},
    {"help",        &shell_command_help},
    {"exit",        &shell_command_exit},
    {"start",       &shell_command_start},
    {"cd",          &shell_command_cd},
    {"set",         &shell_command_set},
    {"source",      &shell_command_source},
    {"clear",       &shell_command_clear},
#ifdef CONFIG_SHELL_BUILTIN_ONLY
    {"bincat",      &shell_command_bincat},
    {"cat",         &shell_command_cat},
    {"date",        &shell_command_date},
    {"date",        &shell_command_date},
    {"echo",        &shell_command_echo},
    {"free",        &shell_command_free},
    {"kill",        &shell_command_kill},
    {"ls",          &shell_command_ls},
    {"mkdir",       &shell_command_mkdir},
    {"pipe",        &shell_command_pipe},
    {"ps",          &shell_command_ps},
    {"pstree",      &shell_command_pstree},
    {"pwd",         &shell_command_pwd},
    {"symlink",     &shell_command_symlink},
    {"readlink",    &shell_command_readlink},
    {"dbg_st",      &shell_command_dbg_st},
    {"cp",          &shell_command_cp},
    {"ln",          &shell_command_ln},
    {"rm",          &shell_command_rm},
    {"stat",        &shell_command_stat},
    {"sleep",       &shell_command_sleep},
    {"bench",       &shell_command_bench},
    {"read",        &shell_command_read},
    {"sync",        &shell_command_sync},
#endif
    {NULL,          NULL}
};


int main(int argc, char* argv[])
{
    tms_init();

    // Startskript ausfuehren
    shell_script("file:/config/shell/start.lsh");

    // Keine Argumente wurden uebergeben, also wird die shell interaktiv
    // aufgerufen.
    if (argc == 1) {
        completion_init();
        while (true) {
            shell_read_command();
            shell_do_cmd();
        }
    } else if ((argc == 2) && (strcmp(argv[1], "--help") == 0)) {
        puts(TMS(usage,
            "Aufruf:\n"
            "  Interaktiv: sh\n"
            "  Skript ausfuehren: sh <Dateiname>\n"
            "  Einzelnen Befehl ausfuehren: sh -c <Befehl>\n"
        ));

    } else if (argc == 2) {
        // Wenn nur ein Argument uebergeben wird, wird das Argument als
        // Dateiname fuer ein Shell-Skript benutzt.
        if (!shell_script(argv[1])) {
            puts(TMS(script_error, "Beim Ausfuehren des Skripts ist ein"
                " Fehler aufgetreten."));
        }
    } else if ((argc == 3) && (strcmp(argv[1], "-c") == 0)) {
        strcpy(shell_command_buffer, argv[2]);
        shell_do_cmd();
    }

    return 0;
}

///Prompt anzeigen und Shellkommando von der Tastatur einlesen
void shell_read_command(void)
{
    char* cwd = getcwd(NULL, 0);
    char* prompt;
    char* input;
    int i;

    asprintf(&prompt, "%s # ", cwd);

    input = readline(prompt);
    if (input) {
        if (*input) {
            add_history(input);
        }

        for (i = strlen(input) - 1; i >= 0; i--) {
            if (input[i] != ' ') {
                input[i+1] = 0;
                break;
            }
        }

        if ((i == 0) && (input[0] == ' ')) {
            input[0] = 0;
        }


        strncpy(shell_command_buffer, input, COMMAND_BUFFER_SIZE);
        free(input);
    } else {
        // Bei EOF wird die Shell beendet
        strcpy(shell_command_buffer, "exit");
    }

    free(prompt);
    free(cwd);
}

/**
 * Testet ob der Befehl am Anfang der uebergebenen Kommandozeile steht.
 *
 * @param cmd Pointer auf die Befehlsbeschreibung.
 * @param cmdline Die eingegebene Kommandozeile.
 *
 * @return true wenn der Befehl drin steht, sonst false
 */
bool shell_match_command(const char* cmd, const char* cmdline)
{
    while (true)
    {
        // Wenn das Ende des Befehlsnamens erreicht ist, muss in der
        // Kommandozeile auch ein Nullbyte oder ein Leerschlag stehen.
        if ((*cmd == '\0') && ((*cmdline == '\0') ||(*cmdline == ' '))) {
            return true;
        } else if (*cmd != *cmdline) {
            return false;
        }
        cmd++;
        cmdline++;
    }

    // Kommt nie vor.
    return false;
}

/**
 * Alle Vorkommen von einer Zeichenkette im Puffer ersetzen
 */
static void buffer_replace(const char* search, const char* replace)
{
    char* res;
    int search_len = strlen(search);
    int replacement_len = strlen(replace);

    while ((res = strstr(shell_command_buffer, search))) {
        // Was uebrig bleibt verschieben
        memmove(res + replacement_len, res + search_len,
            strlen(res + search_len) + 1);

        memcpy(res, replace, replacement_len);
    }
}

/**
 * Umgebungsvariablen im Puffer ersetzen
 */
static void substitute_envvars(void)
{
    int i;
    const char* name;
    const char* value;

    for (i = 0; i < getenv_count(); i++) {
        name = getenv_name_by_index(i);
        value = getenv_index(i);

        // Suchmuster generieren
        char pattern[strlen(name) + 2];
        pattern[0] = '$';
        strcpy(pattern + 1, name);

        buffer_replace(pattern, value);
    }
}

int shell_do_cmd(void)
{
    shell_command_t* command;
    uint32_t i;
    char* cmdstring = shell_command_buffer;

    substitute_envvars();

    // Einrueckung ignorieren
    while (*cmdstring == ' ' || *cmdstring == '\t') {
        cmdstring++;
    }

    // Kommentarzeilen ignorieren
    if (*cmdstring == '#') {
        return 0;
    }


    // Die Liste mit den Befehlen durchsuchen. Das wird solange gemacht, bis
    // der NULL eintrag am Ende erreicht wird.
    for (i = 0; (command = &shell_commands[i]) && (command->name); i++)
    {
        command = &shell_commands[i];
        if (shell_match_command(command->name, cmdstring) == true) {
            char args[strlen(cmdstring) + 1];
            memcpy(args, cmdstring, strlen(cmdstring) + 1);

            int argc, pos;
            argc = 1;
            //Jetzt werden die Argumente gezaehlt.
            for (pos = 0; pos < strlen(args); pos++) {
                if (args[pos] == ' ') {
                    argc++;
                }
            }

            char* argv[argc];
            argv[0] = strtok(args, " ");
            for(pos = 1; pos < argc; pos++)
            {
                argv[pos] = strtok(NULL, " ");
            }

            while (argv[argc - 1] == NULL) {
                argc--;
            }

            return command->handler(argc, argv, cmdstring);
        }
    }
    return shell_command_default(0, 0, cmdstring);
}
/**
 * Shell-Skript ausfuehren
 *
 * @param path Pfad zum Skript
 *
 * @return true bei Erfolg, false sonst
 */
bool shell_script(const char* path)
{
    FILE* script = fopen(path, "r");
    size_t i;

    if (!script) {
        return false;
    }

    while (feof(script) == 0) {
        fgets(shell_command_buffer, COMMAND_BUFFER_SIZE, script);

        i = strlen(shell_command_buffer);
        if ((i > 0) && (shell_command_buffer[i - 1] == '\n')) {
            shell_command_buffer[i - 1] = '\0';
        }

        shell_do_cmd();
    }

    fclose(script);

    return true;
}

