/*
 * Copyright (c) 2006-2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Burkhard Weseloh.
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
#include <lost/config.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

size_t strnlen(const char *s, size_t maxlen)
{
    size_t len = 0;

    while (maxlen-- && *s++) {
        len++;
    }

    return len;
}

//Mit Erlaubnis aus lightOS kopiert
void itoa(unsigned int n, char *s, unsigned int base)
{
	char st[33];
	unsigned int i = 0;
	
	do
	{
		unsigned int t = n % base;
		if (t < 10)st[i++] = t + '0';
		else st[i++] = (t - 10) + 'A';
	} while((n /= base) > 0);
	st[i] = '\0';
	
	unsigned int y = 0;
	for (;i > 0;i--)
	{
		s[y] = st[i-1];
		y++;
	}
	s[y] = '\0';
}

unsigned long long int strtoull(const char *nptr, char **endptr, int base)
{
    unsigned long long int result = 0;
    int neg = 0;
    int digit;
    bool overflow = false;

    // Leerzeichen am Anfang muessen ignoriert werden
    while (isspace(*nptr)) {
        nptr++;
    }

    // Vorzeichen
    if (*nptr == '+') {
        nptr++;
    } else if (*nptr == '-') {
        neg = 1;
        nptr++;
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
            if (*nptr == '0') {
                nptr++;
                if (*nptr == 'x' || *nptr == 'X') {
                    if (isxdigit(nptr[1])) {
                        base = 16;
                        nptr++;
                    }
                } else {
                    base = 8;
                }
            }
            break;

        case 16:
            if ((*nptr == '0') && (nptr[1] == 'x' || nptr[1] == 'X')
                && isxdigit(nptr[2]))
            {
                nptr += 2;
            }
            break;
    }

    // Ergebnis berechnen
    result = 0;
    while (*nptr) {
        switch (*nptr) {
            case '0' ... '9':
                if (*nptr - '0' > base - 1) {
                    goto out;
                }
                digit = *nptr - '0';
                break;
            case 'A' ... 'Z':
                if (*nptr - 'A' + 10 > base - 1) {
                    goto out;
                }
                digit = *nptr - 'A' + 10;
                break;
            case 'a' ... 'z':
                if (*nptr - 'a' + 10 > base - 1) {
                    goto out;
                }
                digit = *nptr - 'a' + 10;
                break;

            default:
                goto out;
        }

        if (result > (ULLONG_MAX - digit) / base) {
            overflow = true;
        }
        result = result * base + digit;

        nptr++;
    }
out:

    if (endptr != NULL) {
        *endptr = (char*) nptr;
    }

    if (overflow) {
        return ULLONG_MAX;
    }

    return neg ? - result : result;
}

unsigned long int strtoul(const char *nptr, char **endptr, int base)
{
    return (unsigned long int) strtoull(nptr, endptr, base);
}


int atoi(const char *s)
{
    return (int) strtoull(s, NULL, 10);
}
//

long atol(const char *str)
{
    if (!*str) {
        return 0;
    }
    bool positive = true;
    long result = 0;
    int length = 0;
    //Vorzeichen
    if (*str == '-') {
        positive = false;
        str++;
    } else if (*str == '+') {
        str++;
    }
    //Zeichenkette auswerten
    while (*str)
    {
        char c = *str;
        if ((c >= '0') && (c <= '9')) {
            length++;
            //Ziffer hinzufügen
            result = result * 10 + c - '0';
        } else {
            break;
        }
        str++;
    }

    if (!length) {
        return 0;
    }

    // Vorzeichen anwenden
    if (!positive) {
        result = -result;
    }

    return result;
}

long double strtold(const char* nptr, char** end)
{
    long double result = 0;
    long double frac = 0;
    long double base = 10.0;
    int neg = 0;
    int decimal = 0;

    // Leerzeichen am Anfang muessen ignoriert werden
    while (isspace(*nptr)) {
        nptr++;
    }

    // Vorzeichen
    if (*nptr == '+') {
        nptr++;
    } else if (*nptr == '-') {
        neg = 1;
        nptr++;
    }

    // Ergebnis berechnen
    result = 0;
    while (*nptr) {
        switch (*nptr) {
            case '0' ... '9':
                if (*nptr - '0' > base - 1) {
                    goto out;
                }
                if (!decimal) {
                    result = result * base + (*nptr - '0');
                } else {
                    frac = frac + ((*nptr - '0') / base);
                    base *= 10.0;
                }
                break;

            case '.':
                decimal = 1;
                break;

            case 'E':
            case 'e':
                // TODO

            default:
                goto out;
        }
        nptr++;
    }
out:
    result += frac;

    if (end != NULL) {
        *end = (char*) nptr;
    }

    return neg ? - result : result;
}

double strtod(const char* nptr, char** end)
{
    return strtold(nptr, end);
}

float strtof(const char* nptr, char** end)
{
    return strtold(nptr, end);
}

/**
 * Zwei Strings unter Beruecksichtigung des aktuellen locales vergleichen
 */
int strcoll(const char* s1, const char* s2)
{
    // TODO: Sobald wir locales haben, muss das nicht mehr umbedingt so sein
    return strcmp(s1, s2);
}

#ifndef CONFIG_LIBC_NO_STUBS
double atof(const char* str)
{
    // TODO
    return 0;
}
#endif

