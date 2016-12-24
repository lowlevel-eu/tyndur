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

#ifndef _SHELL_H_
#define _SHELL_H_

#include <stdbool.h>
#include <wordexp.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/* Tokenizer */
enum token_type {
    TT_END_OF_INPUT,
    TT_WORD,
    TT_OPERATOR,
};

struct token {
    enum token_type type;
    char* value;
    bool is_argv;
    wordexp_t we;
};

struct tokenizer;

struct tokenizer* tokenizer_create(const char *str);
int tokenizer_get(struct tokenizer* tok, struct token* token);
void tokenizer_free(struct tokenizer* tok);

/** Array mit den Befehlen */
typedef struct shell_command_t {
    const char* name;
    int (*handler)(int argc, char* argv[]);
} shell_command_t;

extern shell_command_t shell_commands[];


bool shell_script(const char* path);
bool shell_match_command(const char* cmd, const char* cmdline);
void completion_init(void);

int shell_command_default(int argc, char* argv[]);
int shell_command_help(int argc, char* argv[]);
int shell_command_version(int argc, char* argv[]);
int shell_command_exit(int argc, char* argv[]);
int shell_command_start(int argc, char* argv[]);
int shell_command_cd(int argc, char* argv[]);
int shell_command_set(int argc, char* argv[]);
int shell_command_source(int argc, char* argv[]);
int shell_command_clear(int argc, char* argv[]);


#include <lost/config.h>
#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_bincat(int argc, char* argv[]);
    int shell_command_cat(int argc, char* argv[]);
    int shell_command_date(int argc, char* argv[]);
    int shell_command_echo(int argc, char* argv[]);
    int shell_command_free(int argc, char* argv[]);
    int shell_command_kill(int argc, char* argv[]);
    int shell_command_killall(int argc, char* argv[]);
    int shell_command_ls(int argc, char* argv[]);
    int shell_command_mkdir(int argc, char* argv[]);
    int shell_command_pipe(int argc, char* argv[]);
    int shell_command_ps(int argc, char* argv[]);
    int shell_command_pstree(int argc, char* argv[]);
    int shell_command_pwd(int argc, char* argv[]);
    int shell_command_symlink(int argc, char* argv[]);
    int shell_command_readlink(int argc, char* argv[]);
    int shell_command_dbg_st(int argc, char* argv[]);
    int shell_command_cp(int argc, char* argv[]);
    int shell_command_ln(int argc, char* argv[]);
    int shell_command_rm(int argc, char* argv[]);
    int shell_command_stat(int argc, char* argv[]);
    int shell_command_sleep(int argc, char* argv[]);
    int shell_command_bench(int argc, char* argv[]);
    int shell_command_read(int argc, char* argv[]);
    int shell_command_sync(int argc, char* argv[]);
#endif

#endif

