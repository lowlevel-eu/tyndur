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

#include <wordexp.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

static void skip_quotes(char** s, char **out)
{
    char* p = *s;
    enum {
        UNQUOTED,
        IN_SINGLE_QUOTE,
        IN_DOUBE_QUOTE,
    } state = UNQUOTED;

    while (*p) {
        switch (*p) {
            case '"':
                if (state == UNQUOTED) {
                    state = IN_DOUBE_QUOTE;
                    break;
                } else if (state == IN_DOUBE_QUOTE) {
                    state = UNQUOTED;
                    break;
                }
                goto normal_char;
            case '\'':
                if (state == UNQUOTED) {
                    state = IN_SINGLE_QUOTE;
                    break;
                } else if (state == IN_SINGLE_QUOTE) {
                    state = UNQUOTED;
                    break;
                }
                goto normal_char;
            case '\\':
                if (p[1]) {
                    p++;
                    if (out) {
                        *(*out)++ = *p;
                    }
                }
                break;
            normal_char:
            default:
                if (state == UNQUOTED) {
                    goto out;
                }
                if (out) {
                    *(*out)++ = *p;
                }
                break;
        }

        p++;
    }

out:
    *s = p;
}

static int name_length(const char* s)
{
    const char* start = s;

    if (!*s || !(isalpha(*s) || *s == '_')) {
        return 0;
    }

    while (*s && (isalnum(*s) || *s == '_')) {
        s++;
    }

    return s - start;
}

static int buffer_replace(char** s, char** pos, size_t len, const char* value)
{
    size_t offset = *pos - *s;
    size_t old_total_len = strlen(*s) + 1;
    size_t new_len = strlen(value);
    size_t new_total_len = old_total_len - len + new_len;
    char* p;

    if (len < new_len) {
        p = realloc(*s, new_total_len);
        if (p == NULL) {
            return WRDE_NOSPACE;
        }
        memmove(p + offset + new_len,
                p + offset + len,
                old_total_len - (offset + len));
    } else if (len > new_len) {
        p = *s;
        memmove(p + offset + new_len,
                p + offset + len,
                old_total_len - (offset + len));
        p = realloc(*s, new_total_len);
        if (p == NULL) {
            return WRDE_NOSPACE;
        }
    } else {
        p = *s;
    }

    memcpy(p + offset, value, new_len);
    *s = p;
    *pos = p + offset + new_len - 1;

    return 0;
}

static int we_expand_parameter(char** s)
{
    char* p;
    char* name;
    const char* value;
    size_t len;
    bool in_double_quote = false;
    int ret;

    for (p = *s; *p; p++) {
        if ((!in_double_quote && *p == '\'') || *p == '\\') {
            skip_quotes(&p, NULL);
            if (*p == '\0') {
                break;
            }
        } else if (*p == '"') {
            in_double_quote = !in_double_quote;
        }
        if (*p != '$') {
            continue;
        }

        len = name_length(p + 1);
        if (len == 0) {
            continue;
        }

        name = strndup(p + 1, len);
        if (name == NULL) {
            return WRDE_NOSPACE;
        }

        value = getenv(name);
        if (value == NULL) {
            value = "";
        }

        free(name);

        ret = buffer_replace(s, &p, len + 1, value);
        if (ret != 0) {
            return ret;
        }
    }

    return 0;
}

static int we_split_fields(char** s, wordexp_t* we)
{
    const char* ifs;
    char* p;
    int i;

    if (**s == '\0') {
        free(*s);
        we->we_wordc = 0;
        return 0;
    }
    we->we_wordc = 1;

    ifs = getenv("IFS");
    if (ifs == NULL) {
        ifs = " \n\t";
    }

    for (p = *s; *p; p++) {
        skip_quotes(&p, NULL);
        if (*p == '\0') {
            break;
        }
        if (strchr(ifs, *p)) {
            we->we_wordc++;
        }
    }

    we->we_wordv = calloc(we->we_wordc, sizeof(char*));
    if (we->we_wordv == NULL) {
        return WRDE_NOSPACE;
    }

    we->we_wordv[0] = *s;
    for (p = *s, i = 0; *p; p++) {
        skip_quotes(&p, NULL);
        if (*p == '\0') {
            break;
        }
        if (strchr(ifs, *p)) {
            assert(i < we->we_wordc);
            we->we_wordv[++i] = p + 1;
            *p = '\0';
        }
    }

    for (i = 0; i < we->we_wordc; i++) {
        p = strdup(we->we_wordv[i]);
        if (p == NULL) {
            return WRDE_NOSPACE;
        }
        we->we_wordv[i] = p;
    }

    return 0;
}

static void we_str_remove_quotes(char** s)
{
    char *src;
    char *out;

    for (src = *s, out = *s; *src; src++, out++) {
        skip_quotes(&src, &out);
        if (*src == '\0') {
            break;
        }
        if (src != out) {
            *out = *src;
        }
    }

    if (src != out) {
        size_t len;
        *out++ = '\0';
        len = out - *s;
        out = realloc(*s, len);
        if (out != NULL) {
            *s = out;
        }
    }
}


static void we_remove_quotes(wordexp_t* we)
{
    int i;

    for (i = 0; i < we->we_wordc; i++) {
        we_str_remove_quotes(&we->we_wordv[i]);
    }
}

int wordexp(const char* word, wordexp_t* we, int flags)
{
    char *s;
    int ret;

    s = strdup(word);
    if (s == NULL) {
        return WRDE_NOSPACE;
    }

    ret = we_expand_parameter(&s);
    if (ret != 0) {
        free(s);
        return ret;
    }

    ret = we_split_fields(&s, we);
    if (ret != 0) {
        free(s);
        return ret;
    }

    we_remove_quotes(we);

    return 0;
}

void wordfree(wordexp_t* we)
{
    int i;

    for (i = 0; i < we->we_wordc; i++) {
        free(we->we_wordv[i]);
    }
    free(we->we_wordv);
}
