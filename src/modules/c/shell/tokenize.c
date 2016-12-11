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

#define TOKENIZER_OUTPUT(c) \
    do { \
        if (tokens) { \
            if (output_size <= p - *output) { \
                char *old_output = *output;\
                int i; \
                output_size *= 2; \
                *output = realloc(*output, output_size); \
                for (i = 0; i < num_tokens; i++) \
                    tokens[i].value = *output + (tokens[i].value - old_output); \
                p = *output + (p - old_output); \
                p_start = *output + (p_start - old_output); \
            } \
            *p++ = (c); \
        } else {\
            p++; \
        } \
    } while(0)

#define TOKENIZER_ACCEPT(token_type) \
    do { \
        if (p != p_start) { \
            TOKENIZER_OUTPUT('\0'); \
            if (tokens) { \
                tokens[num_tokens] = (struct token) { \
                    .type   = token_type, \
                    .value  = p_start, \
                }; \
            } \
            p_start = p; \
            num_tokens++; \
        } \
    } while(0);

#define TOKENIZER_IGNORE() \
    do { \
        p_start = p; \
    } while(0);

static bool valid_operator(const char* buf)
{
    const char* operators[] = {
        "<", ">", ">>", "|",
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(operators); i++) {
        if (!strcmp(buf, operators[i])) {
            return true;
        }
    }

    return false;
}

int tokenize_cmdline(const char* str, char** output, struct token* tokens)
{
    int num_tokens = 0;
    char* p = NULL;
    char* p_start = NULL;
    size_t output_size = 0;

    enum state {
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

    enum state state = WHITESPACE;

    if (output) {
        output_size = strlen(str) + 5;
        p = malloc(output_size);
        if (!p) {
            return -ENOMEM;
        }
        *output = p;
    }

    p_start = p;
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
                TOKENIZER_OUTPUT(*str);
                break;
            case NORMAL:
                switch (*str) {
                    case '\'':   TOKENIZER_OUTPUT(*str); state = SINGLE_QUOTED; break;
                    case '"':   TOKENIZER_OUTPUT(*str); state = QUOTED; break;
                    case '\\':  TOKENIZER_OUTPUT(*str); state = ESCAPED; break;
                    case '<':   TOKENIZER_ACCEPT(TT_WORD); state = OPERATOR; continue;
                    case '>':   TOKENIZER_ACCEPT(TT_WORD); state = OPERATOR; continue;
                    default:
                        if (isspace(*str)) {
                            TOKENIZER_ACCEPT(TT_WORD);
                            state = WHITESPACE;
                        } else {
                            TOKENIZER_OUTPUT(*str);
                        }
                        break;
                }
                break;

            case WHITESPACE:
                if (*str == '#') {
                    TOKENIZER_IGNORE();
                    state = COMMENT;
                } else if (*str == '|') {
                    TOKENIZER_IGNORE();
                    state = OPERATOR;
                    continue;
                } else if (!isspace(*str)) {
                    TOKENIZER_IGNORE();
                    state = NUMBER;
                    continue;
                }
                break;

            case COMMENT:
                break;

            case ESCAPED:
                TOKENIZER_OUTPUT(*str);
                state = NORMAL;
                break;

            case SINGLE_QUOTED:
                if (*str == '\'') {
                    state = NORMAL;
                } else if (*str == '\\') {
                    state = ESCAPED_SINGLE_QUOTE;
                }
                TOKENIZER_OUTPUT(*str);
                break;

            case QUOTED:
                if (*str == '"') {
                    state = NORMAL;
                } else if (*str == '\\') {
                    state = ESCAPED_QUOTE;
                }
                TOKENIZER_OUTPUT(*str);
                break;

            case ESCAPED_QUOTE:
                TOKENIZER_OUTPUT(*str);
                state = QUOTED;
                break;

            case ESCAPED_SINGLE_QUOTE:
                TOKENIZER_OUTPUT(*str);
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
                        TOKENIZER_OUTPUT(*str);
                        str++;
                    } else {
                        break;
                    }
                }

                TOKENIZER_ACCEPT(TT_OPERATOR);
                state = NORMAL;
                continue;
            }
        }

        str++;
    }

    switch (state) {
        case NUMBER:
        case NORMAL:
        case ESCAPED:
            TOKENIZER_ACCEPT(TT_WORD);
            break;
        case WHITESPACE:
        case COMMENT:
            TOKENIZER_IGNORE();
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

    return num_tokens;
}
