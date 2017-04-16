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

    asprintf(&prompt, "\033[1m%s #\033[0m ", cwd);

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

#define parser_new(var) \
    ({ int ret; var = calloc(1, sizeof(*var)); \
       if (!var) { fprintf(stderr, "sh: %s\n", strerror(ENOMEM)); \
                   ret = -ENOMEM; } \
       else { ret = 0; } \
       ret; })

struct parser_str_list_entry {
    char* value;
    struct parser_str_list_entry* next;
};

struct parser_str_list {
    struct parser_str_list_entry* head;
    struct parser_str_list_entry** plast;
};

struct parser_str_list* parser_str_list_create(void)
{
    struct parser_str_list* list;

    if (parser_new(list) < 0) {
        return NULL;
    }

    *list = (struct parser_str_list) {
        .head   = NULL,
        .plast  = &list->head,
    };

    return list;
}

int parser_str_list_add(struct parser_str_list* list, char* str)
{
    struct parser_str_list_entry* entry;
    int ret;

    ret = parser_new(entry);
    if (ret < 0) {
        return ret;
    }

    *entry = (struct parser_str_list_entry) {
        .value  = str,
        .next   = NULL,
    };

    *list->plast = entry;
    list->plast = &entry->next;

    return 0;
}

struct parser_node_redir {
    /* Parser-Ausgabe */
    int fd;
    int flags;
    char* filename;
    struct parser_node_redir* next;

    /* Ausführung */
    lio_stream_t orig_fd;
};

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

int pnt_redirection_parse(struct tokenizer* tok, struct token* t,
                          struct parser_node_redir** result)
{
    struct parser_node_redir* node;
    int fd, flags, ret;

    if (!is_redirection(t->value, &fd, &flags)) {
        return 0;
    }

    ret = parser_new(node);
    if (ret < 0) {
        goto done;
    }
    *node = (struct parser_node_redir) {
        .fd     = fd,
        .flags  = flags,
    };

    free(t->value);

    ret = tokenizer_get(tok, t);
    if (ret < 0 || t->type != TT_WORD) {
        fprintf(stderr, "Umleitungsziel erwartet\n");
        ret = -EINVAL;
        goto done;
    }

    node->filename = t->value;

    ret = tokenizer_get(tok, t);
done:
    if (ret < 0) {
        free(node);
    } else {
        *result = node;
        ret = 1;
    }
    return ret;
}

int pnt_redirection_pre_execute(struct parser_node_redir* node)
{
    const char* path = node->filename;
    lio_resource_t r;
    lio_stream_t to_replace;
    lio_stream_t fd;
    int ret;

    r = lio_resource(path, 1);
    if (r < 0 && (node->flags & LIO_WRITE)) {
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
        return r;
    }

    fd = lio_open(r, node->flags);
    if (fd < 0) {
        fprintf(stderr, "Kann '%s' nicht öffnen: %s\n",
                path, strerror(-fd));
        return fd;
    }

    to_replace = get_stdio_fd(node->fd);
    node->orig_fd = lio_dup(to_replace, -1);

    if (node->fd == 1) {
        fflush(stdout);
    }

    ret = lio_dup(fd, to_replace);
    lio_close(fd);
    if (ret < 0) {
        fprintf(stderr,
                TMS(fail_dup, "lio_dup() fehlgeschlagen: %s\n"),
                strerror(-ret));
        return ret;
    }

    return 0;
}

int pnt_redirection_post_execute(struct parser_node_redir* node)
{
    lio_stream_t to_replace;
    int ret;

    to_replace = get_stdio_fd(node->fd);

    if (node->fd == 1) {
        fflush(stdout);
    }

    ret = lio_dup(node->orig_fd, to_replace);
    if (ret < 0) {
        fprintf(stderr, "lio_dup() fehlgeschlagen: %s\n",
                strerror(-ret));
    }

    lio_close(node->orig_fd);

    return 0;
}

void pnt_redirection_free(struct parser_node_redir* node)
{
    free(node->filename);
    free(node);
}

struct parser_node_cmd {
    struct parser_str_list* args;
    struct parser_node_redir* redir;
    struct parser_node_cmd* next;
};

int pnt_command_parse(struct tokenizer* tok, struct token* t,
                      struct parser_node_cmd** result)
{
    struct parser_node_cmd* node;
    struct parser_node_redir** redir_pnext;
    bool matched = false;
    int ret;

    ret = parser_new(node);
    if (ret < 0) {
        goto done;
    }

    *node = (struct parser_node_cmd) {
        .args   = parser_str_list_create(),
        .redir  = NULL,
    };
    redir_pnext = &node->redir;

    if (!node->args) {
        ret = -ENOMEM;
        goto done;
    }

    while (ret == 0) {
        switch (t->type) {
            case TT_WORD:
            {
                wordexp_t we;
                int i;

                ret = wordexp(t->value, &we, 0);
                if (ret != 0) {
                    fprintf(stderr, TMS(parse_error, "Parserfehler in '%s'\n"),
                            t->value);
                    ret = -EINVAL;
                    goto done;
                }

                for (i = 0; i < we.we_wordc; i++) {
                    ret = parser_str_list_add(node->args, strdup(we.we_wordv[i]));
                    if (ret < 0) {
                        wordfree(&we);
                        goto done;
                    }
                }
                wordfree(&we);
                free(t->value);

                ret = tokenizer_get(tok, t);
                break;
            }
            case TT_OPERATOR:
                ret = pnt_redirection_parse(tok, t, redir_pnext);
                if (ret == 1) {
                    redir_pnext = &(*redir_pnext)->next;
                } else {
                    goto done;
                }
                break;
            default:
                goto done;
        }
        if (ret >= 0) {
            matched = true;
        }
    }
    ret = 0;
done:
    if (ret < 0) {
        free(node);
    } else {
        *result = node;
        ret = matched;
    }
    return ret;
}

int pnt_command_execute(struct parser_node_cmd* node)
{
    int argc;
    struct parser_str_list_entry* p;
    struct parser_node_redir* r;
    char** argv = NULL;
    int i;

    argc = 0;
    for (p = node->args->head; p; p = p->next) {
        argc++;
    }

    /* Leerzeilen ignorieren */
    if (argc == 0) {
        return 0;
    }

    /* argv befüllen */
    argv = calloc(argc + 1, sizeof(*argv));
    if (argv == NULL) {
        fprintf(stderr, "Interner Fehler: Kein Speicher fuer argv\n");
        return 0;
    }

    for (i = 0, p = node->args->head; i < argc; i++, p = p->next) {
        argv[i] = p->value;
    }

    /* Passenden Befehl suchen und ausfuehren */
    for (i = 0; shell_commands[i].handler!= NULL; i++) {
        if ((shell_commands[i].name == NULL) || !strcmp(shell_commands[i].name, argv[0])) {
            goto found;
        }
    }
    fprintf(stderr, "Interner Fehler: Kein Befehl gefunden\n");
    goto out;

found:
    for (r = node->redir; r; r = r->next) {
        int ret = pnt_redirection_pre_execute(r);
        if (ret < 0) {
            struct parser_node_redir* s;
            for (s = node->redir; s != r; s = s->next) {
                pnt_redirection_post_execute(s);
            }
            goto out;
        }
    }
    shell_commands[i].handler(argc, argv);
    for (r = node->redir; r; r = r->next) {
        pnt_redirection_post_execute(r);
    }
out:
    free(argv);
    return 0;
}

void pnt_command_free(struct parser_node_cmd* node)
{
    struct parser_str_list_entry* p;
    struct parser_str_list_entry* next_p;
    struct parser_node_redir* r;
    struct parser_node_redir* next_r;

    for (p = node->args->head; p; p = next_p) {
        next_p = p->next;
        free(p->value);
        free(p);
    }
    free(node->args);

    for (r = node->redir; r; r = next_r) {
        next_r = r->next;
        pnt_redirection_free(r);
    }

    free(node);
}

struct parser_node_pipeline {
    struct parser_node_cmd* cmd;
};

int pnt_pipeline_parse(struct tokenizer* tok, struct token* t,
                       struct parser_node_pipeline** result)
{
    struct parser_node_pipeline* node;
    struct parser_node_cmd** cmd_pnext;
    bool matched = false;
    int ret;

    ret = parser_new(node);
    if (ret < 0) {
        goto done;
    }

    cmd_pnext = &node->cmd;

    ret = pnt_command_parse(tok, t, cmd_pnext);
    if (ret <= 0) {
        goto done;
    }
    matched = true;
    cmd_pnext = &(*cmd_pnext)->next;

    while (t->type == TT_OPERATOR && !strcmp(t->value, "|")) {
        free(t->value);
        ret = tokenizer_get(tok, t);
        if (ret < 0) {
            goto done;
        }
        ret = pnt_command_parse(tok, t, cmd_pnext);
        if (ret == 0) {
            fprintf(stderr, "Befehl erwartet\n");
            ret = -EINVAL;
        }
        if (ret < 0) {
            goto done;
        }
        cmd_pnext = &(*cmd_pnext)->next;
    }

done:
    if (ret < 0) {
        free(node);
    } else {
        *result = node;
        ret = matched;
    }
    return ret;
}

int pnt_pipeline_execute(struct parser_node_pipeline* node)
{
    struct parser_node_cmd* p = node->cmd;
    lio_stream_t in, out;
    lio_stream_t orig_in, orig_out;
    lio_stream_t pipe_in, pipe_out;
    int ret;

    /* Ein einzelner Befehl kann abkürzen */
    if (p->next == NULL) {
        return pnt_command_execute(p);
    }

    /* Original-stdin/out sichern */
    in = file_get_stream(stdin);
    out = file_get_stream(stdout);

    orig_in = lio_dup(in, -1);
    if (orig_in < 0) {
        return orig_in;
    }
    orig_out = lio_dup(out, -1);
    if (orig_out < 0) {
        return orig_out;
    }

    /* Bis auf das letzte eins nach dem anderen ausführen, die Ausgabe kommt
     * jeweils in eine Pipe, die danach auf stdin umgeleitet wird. */
    while (p->next) {
        ret = lio_pipe(&pipe_in, &pipe_out, false);
        if (ret < 0) {
            goto fail;
        }

        ret = lio_dup(pipe_out, out);
        lio_close(pipe_out);
        if (ret < 0) {
            lio_close(pipe_in);
            goto fail;
        }

        pnt_command_execute(p);

        ret = lio_dup(pipe_in, in);
        lio_close(pipe_in);
        if (ret < 0) {
            goto fail;
        }

        p = p->next;
    }

    ret = 0;
fail:
    /* Der letzte Befehl kann schon wieder auf das Original-stdout schreiben,
     * benutzt aber noch stdin aus der letzten Pipe. */
    lio_dup(orig_out, out);
    lio_close(orig_out);

    if (ret == 0) {
        ret = pnt_command_execute(p);
    }

    lio_dup(orig_in, in);
    lio_close(orig_in);

    return ret;
}

void pnt_pipeline_free(struct parser_node_pipeline* node)
{
    struct parser_node_cmd* p;
    struct parser_node_cmd* next_p;

    for (p = node->cmd; p; p = next_p) {
        next_p = p->next;
        pnt_command_free(p);
    }

    free(node);
}

int handle_command(char* buf)
{
    struct tokenizer* tok;
    struct token t;
    struct parser_node_pipeline* root;
    int ret, result = 0;

    tok = tokenizer_create(buf);
    if (tok == NULL) {
        fprintf(stderr, "Kein freier Speicher\n");
        return 0;
    }

    ret = tokenizer_get(tok, &t);
    if (ret < 0) {
        goto fail;
    }

    ret = pnt_pipeline_parse(tok, &t, &root);
    if (ret < 0) {
        goto fail_parser;
    }

    if (t.type != TT_END_OF_INPUT) {
        free(t.value);
    }

    result = pnt_pipeline_execute(root);
fail_parser:
    pnt_pipeline_free(root);
fail:
    tokenizer_free(tok);
    return result;
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

