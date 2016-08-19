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
#include <errno.h>

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
int     handle_command(char* buf);


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
    {"killall",     &shell_command_killall},
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
    {NULL,          &shell_command_default}
};


int main(int argc, char* argv[])
{
    tms_init();

    // Keine Argumente wurden uebergeben, also wird die shell interaktiv
    // aufgerufen.
    if (argc == 1) {
        // Startskript ausfuehren
        shell_script("file:/config/shell/start.lsh");

        completion_init();
        while (true) {
            shell_read_command();
            handle_command(shell_command_buffer);
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
        handle_command(argv[2]);
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

static bool is_redirection(const char* word, int* stdio_idx, int* flags)
{
    *stdio_idx = -1;

    if (!strcmp(word, "<")) {
        *stdio_idx = 0;
        *flags = LIO_READ;
    } else if (!strcmp(word, ">")) {
        *stdio_idx = 1;
        *flags = LIO_WRITE | LIO_TRUNC;
    } else if (!strcmp(word, ">>")) {
        *stdio_idx = 1;
        *flags = LIO_WRITE; /* TODO append */
    } else if (!strcmp(word, "2>")) {
        *stdio_idx = 2;
        *flags = LIO_WRITE | LIO_TRUNC;
    } else if (!strcmp(word, "2>>")) {
        *stdio_idx = 2;
        *flags =  LIO_WRITE; /* TODO append */
    }

    return (*stdio_idx != -1);
}

static int get_stdio_fd(int index)
{
    switch (index) {
        case 0:
            return file_get_stream(stdin);
        case 1:
            return file_get_stream(stdout);
        case 2:
            return file_get_stream(stderr);
    }

    abort();
    return -1;
}

int handle_command(char* buf)
{
    char* args;
    char** argv = NULL;
    int num_tokens, argc;
    int i;
    int in_idx, out_idx;
    struct token* tokens;
    int stdio_orig[3] = { 0 };
    int ret;

    num_tokens = tokenize_cmdline(buf, NULL, NULL);
    if (num_tokens < 0) {
        return 0;
    }

    tokens = calloc(num_tokens, sizeof(*tokens));
    if (num_tokens && tokens == NULL) {
        free(argv);
        fprintf(stderr, "Interner Fehler: Kein Speicher für tokens\n");
        return 0;
    }

    tokenize_cmdline(buf, &args, tokens);

    /* Befehlszeile parsen */
    argc = 0;
    for (i = 0; i < num_tokens; i++) {
        switch (tokens[i].type) {
            case TT_OPERATOR:
            {
                int stdio_idx, flags;
                const char* path;
                lio_resource_t r;
                lio_stream_t fd;
                int stdio_fd;

                if (!is_redirection(tokens[i].value, &stdio_idx, &flags)) {
                    fprintf(stderr, "Unbekannter Operator '%s'\n",
                            tokens[i].value);
                    goto found;
                }

                if (++i >= num_tokens || tokens[i].type != TT_WORD) {
                    fprintf(stderr, "Umleitungsziel erwartet\n");
                    goto found;
                }

                path = tokens[i].value;
                r = lio_resource(path, 1);
                if (r < 0 && (flags & LIO_WRITE)) {
                    lio_resource_t parent;
                    char* dirname = io_split_dirname(path);
                    char* filename = io_split_filename(path);

                    if ((parent = lio_resource(dirname, 1)) > 0) {
                        r = lio_mkfile(parent, filename);
                    }

                    free(dirname);
                    free(filename);
                }
                if (r < 0) {
                    fprintf(stderr, "Kann '%s' nicht öffnen: %s\n",
                            path, strerror(-r));
                }

                fd = lio_open(r, flags);
                if (fd < 0) {
                    fprintf(stderr, "Kann '%s' nicht öffnen: %s\n",
                            path, strerror(-fd));
                    goto found;
                }

               stdio_fd = get_stdio_fd(stdio_idx);
                if (!stdio_orig[stdio_idx]) {
                    stdio_orig[stdio_idx] = lio_dup(stdio_fd, -1);
                }
                fflush(stdout);
                ret = lio_dup(fd, stdio_fd);
                lio_close(fd);
                if (ret < 0) {
                    fprintf(stderr,
                            TMS(fail_dup, "lio_dup() fehlgeschlagen: %s\n"),
                            strerror(-ret));
                    goto found;
                }
                break;
            }

            case TT_WORD:
                ret = wordexp(tokens[i].value, &tokens[i].we, 0);
                if (ret < 0) {
                    fprintf(stderr, TMS(parse_error, "Parserfehler in '%s'\n"),
                            tokens[i].value);
                    goto found;
                }
                tokens[i].is_argv = true;
                argc += tokens[i].we.we_wordc;
                break;

            default:
                abort();
        }
    }

    /* Leerzeilen ignorieren */
    if (argc == 0) {
        goto found;
    }

    /* argv befüllen */
    argv = calloc(argc + 1, sizeof(*argv));
    if (argv == NULL) {
        fprintf(stderr, "Interner Fehler: Kein Speicher für argv\n");
        return 0;
    }

    for (in_idx = out_idx = 0; in_idx < num_tokens; in_idx++) {
        if (tokens[in_idx].is_argv) {
            int j;
            for (j = 0; j < tokens[in_idx].we.we_wordc; j++) {
                argv[out_idx++] = tokens[in_idx].we.we_wordv[j];
            }
        }
    }


    /* Passenden Befehl suchen und ausfuehren */
    for (i = 0; shell_commands[i].handler != NULL; i++) {
        if ((shell_commands[i].name == NULL) ||
            !strcmp(shell_commands[i].name, argv[0]))
        {
            shell_commands[i].handler(argc, argv);
            goto found;
        }
    }

    fprintf(stderr, "Interner Fehler: Kein Befehl gefunden\n");
    return 0;

    /* Aufraeumen */
found:
    for (i = 0; i < num_tokens; i++) {
        if (tokens[i].is_argv) {
            wordfree(&tokens[i].we);
        }
    }
    free(tokens);
    free(args);
    free(argv);

    /* Ausgabeumleitung rückgängig machen */
    for (i = 0; i < 3; i++) {
        if (stdio_orig[i]) {
            int stdio_fd = get_stdio_fd(i);

            fflush(stdout);

            ret = lio_dup(stdio_orig[i], stdio_fd);
            if (ret < 0) {
                fprintf(stderr, "lio_dup() fehlgeschlagen: %s\n",
                        strerror(-ret));
            }
        }
    }

    return 0;
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

        handle_command(shell_command_buffer);
    }

    fclose(script);

    return true;
}

