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

#include "ctype.h"
#include "errno.h"
#include "limits.h"
#include "types.h"

static long long do_strtoll(const char *str, char **endptr, int base,
                            long long min_val, long long max_val)
{
    long long retval = 0;
    int overflow = 0;
    char sign = 0;
    int digit;

    while (isspace(*str)) {
        str++;
    }

    // Moegliches Vorzeichen auswerten
    if (*str == '+' || *str == '-') {
        sign = *str;
        str++;
    }

    // Moegliches 0, 0x und 0X auswerten
    if (*str == '0')
    {
        if (str[1] == 'x' || str[1] == 'X') {
            if (base == 0) {
                base = 16;
            }

            if (base == 16 && isxdigit(str[2])) {
                str += 2;
            }
        } else if (base == 0) {
            base = 8;
        }
    }

    while (*str)
    {
        if (isdigit(*str) && *str - '0' < base) {
            digit = *str - '0';
        } else if(isalpha(*str) && tolower(*str) - 'a' + 10 < base) {
            digit = tolower(*str) - 'a' + 10;
        } else {
            break;
        }

        if (retval > (max_val - digit) / base) {
            overflow = 1;
        }
        retval = retval * base + digit;

        str++;
    }

    if (endptr != NULL) {
        *(const char**)endptr = str;
    }

    if (overflow) {
        errno = ERANGE;
        return (sign == '-') ? min_val : max_val;
    }

    return (sign == '-') ? -retval : retval;
}

long long strtoll(const char *str, char **endptr, int base)
{
    return do_strtoll(str, endptr, base, LLONG_MIN, LLONG_MAX);
}

long strtol(const char *str, char **endptr, int base)
{
    return do_strtoll(str, endptr, base, LONG_MIN, LONG_MAX);
}
