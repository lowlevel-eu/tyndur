/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

typedef int (*jscanf_getc)(void* state);
typedef void (*jscanf_ungetc)(void* state, char c);
typedef int (*jscanf_tell)(void* state);

/**
 * Liest Zahlen passend fuer jscanf ein, d.h. es wird so lange gelesen, wie
 * theoretisch noch eine gueltige Zahl herauskommen koennte. Das Parsen und
 * Entscheiden, ob es tatsaechlich eine gueltige Zahl ist, bleibt dem Aufrufer
 * ueberlassen.
 *
 * @param buf Puffer, in den die Zeichen eingelesen werden
 * @param size Laenge des Puffers in einlesbaren Zeichen. Es muss mindestens
 * Platz fuer size + 1 Zeichen vorhanden sein, da noch ein Nullbyte geschrieben
 * wird.
 * @param jgetc Funktion, um ein Zeichen auszulesen
 * @param jungetc Funktion, um ein Zeichen zurueckzugeben, wenn es doch nicht
 * passt
 * @param state Zustand der Eingabe fuer jgetc/jungetc
 * @param base Basis der einzulesenden Zahl (8, 10 oder 16)
 * @param eof Wird auf 1 gesetzt, wenn die Eingabe wegen EOF beendet wurde, auf
 * 0, wenn ein unpassendes Zeichen gekommen ist
 *
 * @return Anzahl der in den Puffer eingelesenen Zeichen
 */
static int jscanf_read_number(char* buf, int size, jscanf_getc jgetc,
    jscanf_ungetc jungetc, void* state, int base, int* eof)
{
    int valid;
    int i;
    int c;
    int first_digit = 0;

    *eof = 0;
    i = 0;
    while (i < size) {
        c = jgetc(state);

        if (c == EOF) {
            *eof = 1;
            break;
        }

        buf[i] = c;
        valid = 0;
        switch (c) {
            case '+':
            case '-':
                if (i == 0) {
                    valid = 1;
                    first_digit++;
                }
                break;
            case '0':
                valid = 1;
                break;
            case '1' ... '7':
                if (base == 0) {
                    base = buf[first_digit] == '0' ? 8 : 10;
                }
                valid = 1;
                break;
            case '8' ... '9':
                if (base == 0) {
                    base = buf[first_digit] == '0' ? 8 : 10;
                }
                valid = (base != 8);
                break;
            case 'A' ... 'F':
            case 'a' ... 'f':
                valid = (base == 16);
                break;
            case 'x':
                if (base == 0) {
                    base = 16;
                }
                if ((base == 16) && (i == first_digit + 1) &&
                    (buf[first_digit] == '0'))
                {
                    valid = 1;
                }
                break;
        }

        if (!valid) {
            jungetc(state, c);
            break;
        }

        i++;
    }

    buf[i] = '\0';

    return i;
}

/**
 * Weist eine Zahl einer Variablen zu. Die Groesse der Variablen wird dabei
 * als Parameter uebergeben.
 *
 * @param ptr Variable, in die die Zahl geschrieben werden soll
 * @param value Wert, der der Variablen zugewiesen werden soll
 * @param size Groesse der Variablen in Bytes
 */
static void assign_number(void* ptr, uint64_t value, int size)
{
    if (size == 1) {
        uint8_t* tptr = ptr;
        *tptr = (uint8_t) value;
    } else if (size == 2) {
        uint16_t* tptr = ptr;
        *tptr = (uint16_t) value;
    } else if (size == 4) {
        uint32_t* tptr = ptr;
        *tptr = (uint32_t) value;
    } else if (size == 8) {
        uint64_t* tptr = ptr;
        *tptr = (uint64_t) value;
    } else {
        abort();
    }
}

/**
 * Die eigentliche scanf-Engine, die den Formatstring verarbeitet und die
 * eingelesenen Daten in die uebergebenen Parameter schreibt. Die
 * Eingabezeichen werden mit den uebergebenen Funktionen geholt.
 *
 * @param fmt scanf-Formatstring
 * @param ap Liste der Ausgabevariablen
 * @param jgetc Funktion, um ein Zeichen auszulesen
 * @param jungetc Funktion, um ein Zeichen zurueckzugeben, wenn es doch nicht
 * passt
 * @param jtell Funktion, die die Anzahl der bisher gelesenen Zeichen
 * zurueckgibt
 * @param state Zustand der Eingabe fuer jgetc/jungetc
 *
 * @return Anzahl der erfolgreich zugewiesenen Variablen; EOF, wenn ein
 * Eingabefehler auftritt, bevor mindestens eine Variable erfolgreich
 * eingelesen wurde.
 */
static int jscanf(const char* fmt, va_list ap,
    jscanf_getc jgetc, jscanf_ungetc jungetc, jscanf_tell jtell, void* state)
{
    int ret = 0;
    int assign;
    int len;
    int size;
    int c;
    char* endptr;
    uint64_t value;

    // 64 Bit oktal = 22 Zeichen, führende 0, Vorzeichen und \0
    char buf[25];

    while (*fmt) {
        switch (*fmt) {
            case ' ':
            case '\t':
            case '\n':
            case '\f':
            case '\v':
                do {
                    c = jgetc(state);
                } while (isspace(c));


                if (c != EOF) {
                    jungetc(state, c);
                }
                break;

            case '%':
                fmt++;

                // Ein * bedeutet, dass der Wert nur eingelesen, aber keiner
                // Variablen zugewiesen wird
                if (*fmt == '*') {
                    assign = 0;
                    fmt++;
                } else {
                    assign = 1;
                }

                // Optional kann jetzt die Feldlaenge kommen
                if (isdigit(*fmt)) {
                    len = strtol(fmt, (char**) &fmt, 10);
                    if (len == 0) {
                        goto matching_error;
                    }
                } else {
                    len = 0;
                }

                // Und die Laenge der Variablen kann auch noch angegeben sein
                switch (*fmt) {
                    case 'h':
                        if (*++fmt == 'h') {
                            fmt++;
                            size = sizeof(char);
                        } else {
                            size = sizeof(short);
                        }
                        break;
                    case 'l':
                        if (*++fmt == 'l') {
                            fmt++;
                            size = sizeof(long long);
                        } else {
                            size = sizeof(long);
                        }
                        break;
                    case 'j':
                        size = sizeof(intmax_t);
                        break;
                    case 't':
                        size = sizeof(ptrdiff_t);
                        break;
                    case 'z':
                        size = sizeof(size_t);
                        break;
                    default:
                        size = sizeof(int);
                        break;
                }

                // Whitespace muss uebersprungen werden (ausser %[ %c %n)
                if ((*fmt != '[') && (*fmt != 'c') && (*fmt != 'n')) {
                    while (isspace(c = jgetc(state)));
                    if (c != EOF) {
                        jungetc(state, c);
                    }
                }

                // Eingabe parsen
                switch (*fmt) {
                    int base;
                    int eof;

                    // %i - Integer (strtol mit Basis 0)
                    // %d - Integer (strtol mit Basis 10)
                    // %o - Integer (strtoul mit Basis 8
                    // %u - Integer (strtoul mit Basis 1)
                    // %x - Integer (strtoul mit Basis 16)
                    // %X - Integer (strtoul mit Basis 16)
                    // %p - Pointer (strtoul mit Basis 16)
                    case 'i':
                        base = 0;
                        goto convert_number;
                    case 'o':
                        base = 8;
                        goto convert_number;
                    case 'x':
                    case 'X':
                        base = 16;
                        goto convert_number;
                    case 'p':
                        base = 16;
                        size = sizeof(void*);
                        len = 0;
                        goto convert_number;
                    case 'd':
                    case 'u':
                        base = 10;
                    convert_number:
                        if (len == 0 || len >= sizeof(buf)) {
                            len = sizeof(buf) - 1;
                        }
                        len = jscanf_read_number(buf, len, jgetc, jungetc,
                            state, base, &eof);

                        value = strtoull(buf, &endptr, base);
                        if ((endptr != buf + len) || (len == 0)) {
                            if ((len == 0) && eof) {
                                goto input_error;
                            }
                            goto matching_error;
                        }

                        if (assign) {
                            void* ptr = va_arg(ap, void*);
                            assign_number(ptr, value, size);
                            ret++;
                        }
                        break;

                    // %n - Anzahl der bisher gelesenen Zeichen
                    case 'n':
                        if (assign) {
                            void* ptr = va_arg(ap, void*);
                            assign_number(ptr, jtell(state), size);
                        }
                        break;

                    // %c - char-Array (feste Laenge, kein Nullbyte)
                    case 'c':
                    {
                        // TODO Multibyte-Zeichen
                        int i;

                        if (len == 0) {
                            len = 1;
                        }

                        char buf[len];

                        for (i = 0; i < len; i++) {
                            c = jgetc(state);
                            if (c == EOF) {
                                if (i == 0) {
                                    goto input_error;
                                } else {
                                    goto matching_error;
                                }
                            }
                            buf[i] = c;
                        }

                        if (assign) {
                            char* ptr = va_arg(ap, char*);
                            memcpy(ptr, buf, len);
                            ret++;
                        }
                        break;
                    }

                    // %s - Whitespaceterminierter String
                    case 's':
                    {
                        // TODO MUltibyte-Zeichen
                        char* ptr = NULL;
                        int matched = 0;

                        if (assign) {
                            ptr = va_arg(ap, char*);
                        }

                        if (len == 0) {
                            len = -1;
                        }

                        while ((len == -1) || len--) {
                            c = jgetc(state);
                            if (isspace(c)) {
                                jungetc(state, c);
                                break;
                            } else if (c == EOF) {
                                break;
                            }
                            matched = 1;
                            if (assign) {
                                *ptr++ = c;
                            }
                        }

                        // Matching Error ist nicht moeglich, da alle
                        // Leerzeichen bereits vorher weggelesen werden und
                        // ansonsten alle Zeichen akzeptiert werden.
                        if (!matched) {
                            goto input_error;
                        }

                        if (assign) {
                            *ptr = '\0';
                            ret++;
                        }
                        break;
                    }

                    // %[ - String aus einer Zeicheklasse
                    case '[':
                    {
                        const char* p = fmt;
                        const char* end;
                        int negation = 0;
                        int match = 0;
                        char* ptr = NULL;

                        if (assign) {
                            ptr = va_arg(ap, char*);
                        }

                        // Wenn das Scanset mit ^ anfaengt, duerfen die Zeichen
                        // des Scansets _nicht_ vorkommen
                        if (*++p == '^') {
                            negation = 1;
                            p++;
                        }

                        // Scanset zusammenbasteln
                        if (*p == '\0') {
                            abort();
                        }

                        end = strchr(p + 1, ']');
                        if (end == NULL) {
                            abort();
                        }

                        char scanset[end - p + 1];
                        scanset[end - p] = '\0';
                        strncpy(scanset, p, end - p);

                        // Hier kommt das eigentliche Matching
                        if (len == 0) {
                            len = -1;
                        }

                        c = EOF;
                        while ((len == -1) || len--) {
                            c = jgetc(state);
                            if (c == EOF) {
                                break;
                            } else if (!!strchr(scanset, c) == negation) {
                                jungetc(state, c);
                                break;
                            }
                            match = 1;
                            if (assign) {
                                *ptr++ = c;
                            }
                        }

                        if (!match) {
                            if (c == EOF) {
                                goto input_error;
                            } else {
                                goto matching_error;
                            }
                        }

                        if (assign) {
                            *ptr = '\0';
                            ret++;
                        }

                        fmt = end;

                        break;
                    }

                    // %% - Prozentzeichen
                    case '%':
                        goto parse_percent;

                    // Das Verhalten bei ungueltiger Conversion Specification
                    // ist undefiniert.
                    default:
                        abort();
                }
                break;

            // Ein einzelnes Zeichen wird direkt gematcht
            parse_percent:
            default:
                c = jgetc(state);
                if (c == EOF) {
                    goto input_error;
                }
                if (c != *fmt) {
                    goto matching_error;
                }
                break;
        }

        fmt++;
    }

matching_error:
    return ret;

input_error:
    if (ret == 0) {
        return EOF;
    }
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// sscanf - Eingabe aus einem String

struct sscanf_state {
    const char* input;
    int pos;
};

static int sscanf_getc(void* state)
{
    struct sscanf_state* s = state;
    int ret = EOF;

    if (s->input[s->pos]) {
        ret = s->input[s->pos];
        s->pos++;
    }

    return ret;
}

static void sscanf_ungetc(void* state, char c)
{
    struct sscanf_state* s = state;

    if (s->pos > 0) {
        s->pos--;
        if (s->input[s->pos] != c) {
            abort();
        }
    } else {
        abort();
    }
}

static int sscanf_tell(void* state)
{
    struct sscanf_state* s = state;
    return s->pos;
}

/**
 * Fuehrt ein jscanf mit Eingabe aus einem String aus
 */
int vsscanf(const char* input, const char* fmt, va_list ap)
{
    struct sscanf_state state = {
        .input = input,
        .pos = 0,
    };

    return jscanf(fmt, ap, sscanf_getc, sscanf_ungetc, sscanf_tell, &state);
}

/**
 * Fuehrt ein jscanf mit Eingabe aus einem String aus
 */
int sscanf(const char* input, const char* fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = vsscanf(input, fmt, ap);
    va_end(ap);

    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// fscanf - Eingabe aus einer Datei

struct fscanf_state {
    FILE* file;
    int pos;
};

static int fscanf_getc(void* state)
{
    struct fscanf_state* s = state;
    FILE* f = s->file;
    int c;

    c = fgetc(f);
    if (c != EOF) {
        s->pos++;
    }

    return c;
}

static void fscanf_ungetc(void* state, char c)
{
    struct fscanf_state* s = state;
    FILE* f = s->file;

    s->pos--;
    ungetc(c, f);
}

static int fscanf_tell(void* state)
{
    struct fscanf_state* s = state;
    return s->pos;
}

/**
 * Fuehrt ein jscanf mit Eingabe aus einer Datei aus
 */
int vfscanf(FILE* f, const char* fmt, va_list ap)
{
    struct fscanf_state state = {
        .file = f,
        .pos = 0,
    };
    return jscanf(fmt, ap, fscanf_getc, fscanf_ungetc, fscanf_tell, &state);
}

/**
 * Fuehrt ein jscanf mit Eingabe aus einer Datei aus
 */
int fscanf(FILE* f, const char* fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = vfscanf(f, fmt, ap);
    va_end(ap);

    return ret;
}


///////////////////////////////////////////////////////////////////////////////
// scanf - Eingabe aus der Standardeingabe

/**
 * Fuehrt ein jscanf mit Eingabe aus der Standardeingabe aus
 */
int vscanf(const char* fmt, va_list ap)
{
    return vfscanf(stdin, fmt, ap);
}

/**
 * Fuehrt ein jscanf mit Eingabe aus der Standardeingabe aus
 */
int scanf(const char* fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = vscanf(fmt, ap);
    va_end(ap);

    return ret;
}
