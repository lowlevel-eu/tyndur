/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Burkhard Weseloh
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

#include <stdbool.h>
#include "ctype.h"
#include "errno.h"
#include "limits.h"
#include "types.h"

static unsigned long long do_strtoull(const char *str, char **endptr, int base,
                                      unsigned long long max_pos,
                                      unsigned long long max_neg)
{
    unsigned long long result = 0;
    bool is_neg = false;
    int digit;
    int overflow = 0;

    // Leerzeichen am Anfang muessen ignoriert werden
    while (isspace(*str)) {
        str++;
    }

    // Vorzeichen
    if (*str == '+') {
        str++;
    } else if (*str == '-') {
        is_neg = true;
        str++;
    }

    // Basis feststellen
    // Bei Basis 16 darf der String mit 0x anfangen
    // Bei Basis 0 ist automatische Erkennung
    //   Anfang ist 0x: Basis 16
    //   Anfang ist 0:  Basis 8
    //   Ansonsten:     Basis 10
    switch (base) {
        case 0:
            base = 10;
            if (*str == '0') {
                str++;
                if (*str == 'x' || *str == 'X') {
                    if (isxdigit(str[1])) {
                        base = 16;
                        str++;
                    }
                } else {
                    base = 8;
                }
            }
            break;

        case 16:
            if ((*str == '0') && (str[1] == 'x' || str[1] == 'X')
                && isxdigit(str[2]))
            {
                str += 2;
            }
            break;
    }

    // Ergebnis berechnen
    result = 0;
    while (*str) {
        switch (*str) {
            case '0' ... '9':
                if (*str - '0' > base - 1) {
                    goto out;
                }
                digit = *str - '0';
                break;
            case 'A' ... 'Z':
                if (*str - 'A' + 10 > base - 1) {
                    goto out;
                }
                digit = *str - 'A' + 10;
                break;
            case 'a' ... 'z':
                if (*str - 'a' + 10 > base - 1) {
                    goto out;
                }
                digit = *str - 'a' + 10;
                break;

            default:
                goto out;
        }

        if (is_neg) {
            if (result > (max_neg - digit) / base) {
                errno = ERANGE;
                overflow = 1;
            }
        } else {
            if (result > (max_pos - digit) / base) {
                errno = ERANGE;
                overflow = 1;
            }
        }

        result = result * base + digit;

        str++;
    }
out:

    if (endptr != NULL) {
        *endptr = (char*) str;
    }

    if (overflow) {
        return is_neg ? -max_neg : max_pos;
    }

    return is_neg ? -result : result;
}

long long strtoll(const char *str, char **endptr, int base)
{
    return (long long)do_strtoull(str, endptr, base, LLONG_MAX,
                                  (unsigned long long)LLONG_MIN);
}

long strtol(const char *str, char **endptr, int base)
{
    return (long)do_strtoull(str, endptr, base, LONG_MAX,
                             (unsigned long)LONG_MIN);
}

unsigned long long strtoull(const char *str, char **endptr, int base)
{
    return (unsigned long long)do_strtoull(str, endptr, base,
                                           ULLONG_MAX, ULLONG_MAX);
}

unsigned long strtoul(const char *str, char **endptr, int base)
{
    return (unsigned long)do_strtoull(str, endptr, base, ULONG_MAX, ULONG_MAX);
}
