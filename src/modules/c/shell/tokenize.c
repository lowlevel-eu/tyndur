/*
 * Copyright (c) 2016 Kevin Wolf
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include "shell.h"

#define TMS_MODULE shell
#include <tms.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static bool valid_operator(const char* buf)
{
    const char* operators[] = {
        "<", ">", ">>", "|"
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(operators); i++) {
        if (!strcmp(buf, operators[i])) {
            return true;
        }
    }

    return false;
}

enum tokenizer_state {
    WHITESPACE,
    NUMBER,
    NORMAL,
    COMMENT,
    QUOTED,
    SINGLE_QUOTED,
    ESCAPED,
    ESCAPED_QUOTE,
    ESCAPED_SINGLE_QUOTE,
    OPERATOR,
};

struct tokenizer {
    enum tokenizer_state state;
    const char *input;
};

struct tokenizer* tokenizer_create(const char *str)
{
    struct tokenizer* tok;

    tok = malloc(sizeof(*tok));
    if (tok == NULL) {
        return NULL;
    }

    *tok = (struct tokenizer) {
        .state  = WHITESPACE,
        .input  = str,
    };
    return tok;
}

#define TOKENIZER_IGNORE(next_state) \
    do { tok->input = str; state = next_state; goto restart; } while(0);
#define TOKENIZER_ACCEPT(token_type, next_state) \
    do { tok->state = next_state; type = token_type; goto accept; } while(0)

int tokenizer_get(struct tokenizer* tok, struct token* token)
{
    const char* str = tok->input;
    enum tokenizer_state state = tok->state;
    enum token_type type = TT_WORD;

    if (!str || !*str) {
        *token = (struct token) {
            .type   = TT_END_OF_INPUT,
            .value  = NULL,
        };
        return 0;
    }

restart:
    while (*str) {
        switch (state) {
            case NUMBER:
                if (*str == '<' || *str == '>') {
                    state = OPERATOR;
                    continue;
                } else if (!isdigit(*str)) {
                    state = NORMAL;
                    continue;
                }
                break;
            case NORMAL:
                switch (*str) {
                    case '\'': state = SINGLE_QUOTED; break;
                    case '"':  state = QUOTED; break;
                    case '\\': state = ESCAPED; break;
                    case '<':  TOKENIZER_ACCEPT(TT_WORD, OPERATOR);
                    case '>':  TOKENIZER_ACCEPT(TT_WORD, OPERATOR);
                    default:
                        if (isspace(*str)) {
                            TOKENIZER_ACCEPT(TT_WORD, WHITESPACE);
                        }
                        break;
                }
                break;

            case WHITESPACE:
                if (*str == '#') {
                    TOKENIZER_IGNORE(COMMENT);
                } else if (*str == '|') {
                    TOKENIZER_IGNORE(OPERATOR);
                } else if (!isspace(*str)) {
                    TOKENIZER_IGNORE(NUMBER);
                }
                break;

            case COMMENT:
                break;

            case ESCAPED:
                state = NORMAL;
                break;

            case SINGLE_QUOTED:
                if (*str == '\'') {
                    state = NORMAL;
                } else if (*str == '\\') {
                    state = ESCAPED_SINGLE_QUOTE;
                }
                break;

            case QUOTED:
                if (*str == '"') {
                    state = NORMAL;
                } else if (*str == '\\') {
                    state = ESCAPED_QUOTE;
                }
                break;

            case ESCAPED_QUOTE:
                state = QUOTED;
                break;

            case ESCAPED_SINGLE_QUOTE:
                state = SINGLE_QUOTED;
                break;

            case OPERATOR:
            {
                /* Längster Operator ist drei Zeichen */
                char buf[4] = { 0 };
                int i;

                for (i = 0; i < 3; i++) {
                    buf[i] = *str;
                    if (valid_operator(buf)) {
                        str++;
                    } else {
                        break;
                    }
                }

                TOKENIZER_ACCEPT(TT_OPERATOR, WHITESPACE);
                break;
            }
        }

        str++;
    }

    switch (state) {
        case NUMBER:
        case NORMAL:
        case ESCAPED:
            TOKENIZER_ACCEPT(TT_WORD, NUMBER);
            break;
        case WHITESPACE:
        case COMMENT:
            TOKENIZER_IGNORE(NUMBER);
            break;
        case QUOTED:
        case ESCAPED_QUOTE:
            fprintf(stderr, TMS(err_end_dquot,
                                "Abschliessendes '\"' nicht gefunden\n"));
            return -EINVAL;
        case SINGLE_QUOTED:
        case ESCAPED_SINGLE_QUOTE:
            fprintf(stderr, TMS(err_end_squot,
                                "Abschliessendes '\'' nicht gefunden\n"));
            return -EINVAL;
        case OPERATOR:
            abort();
    }
    abort();

accept:
    *token = (struct token) {
        .type   = type,
        .value  = strndup(tok->input, str - tok->input),
    };
    tok->input = str;

    return 0;
}

void tokenizer_free(struct tokenizer* tok)
{
    free(tok);
}
