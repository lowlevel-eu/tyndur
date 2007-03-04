/*
 * Copyright (c) 2006 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
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
 
#include "types.h"
#include "string.h"
#include "stdlib.h"

static unsigned long long divmod(unsigned long long dividend, unsigned int divisor, unsigned int * remainder)
{
    unsigned int highword = dividend >> 32;
    unsigned int lowword = dividend & 0xffffffff;
    unsigned long long quotient;
    unsigned int rem;

    __asm__("div %%ecx\n\t"
        "xchg %%ebx, %%eax\n\t"
        "div %%ecx\n\t"
        "xchg %%edx, %%ebx"
        : "=A"(quotient), "=b"(rem)
        : "a"(highword), "b"(lowword), "c"(divisor), "d"(0)
        );

    if(remainder)
    {
        *remainder = rem;
    }

    return quotient;
}

static void sputc(char** obuf, char c) 
{
    (*(*obuf)++) = c;
}

static void sputs(char** obuf, char* in)
{
    while(*in) {
        (*(*obuf)++) = *in++;
    }
}

/* Gibt eine unsigned 64-bit Zahl mit beliebiger Basis zwischen 2 und 36 aus */
static void sputn(char** obuf, unsigned long long x, int radix, int pad, char padchar)
{
    char b[65];
    char * r = b + 64; // r zeig auf das letzte Zeichen von b[]
    char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    unsigned int remainder;

    if(radix < 2 || radix > 36)
    {
        return;
    }

    *r-- = 0;

    do
    {
        x = divmod(x, radix, &remainder);
        *r-- = digits[remainder];
        pad--;
    }
    while(x > 0);

    while(pad-- > 0)
    {
        sputc(obuf, padchar);
    }

    sputs(obuf, r + 1);
}

/* Hilfsfunktion für kprintf() */
void do_sprintf(char* obuf, const char * format, int ** args)
{
    int pad;
    char padfill;
    signed long long value = 0;

    while(*format)
    {
        if(*format == '%')
        {
            format++;

            pad = 0;
            if(*format == '0')
            {
                padfill = '0';
                format++;
            }
            else
            {
                padfill = ' ';
            }

            while(*format >= '0' && *format <= '9')
            {
                pad = pad * 10 + *format++ - '0';
            }

            if(format[0] == 'l' && format[1] == 'l' && (format[2] == 'd' || format[2] == 'i' || format[2] == 'o' || format[2] == 'u' || format[2] == 'x'))
            {
                value = *(unsigned long long*)(*args);
                *args += 2;

                format += 2;
            }
            else if(format[0] == 'd' || format[0] == 'u')
            {
                value = *(*args)++;
                if(value < 0)
                {
                    sputc(&obuf, '-');
                    pad--;
                    value = -value;
                }
            }
            else if(format[0] == 'i' || format[0] == 'o' || format[0] == 'p' || format[0] == 'x')
            {
                value = *(*args)++;
                value = value & 0xffffffff;
            }

            switch(*format)
            {
                case 0:
                    return;
                case '%':
                    sputc(&obuf, '%');
                    break;
                case 'c':
                    sputc(&obuf, *(*args)++);
                    break;
                case 'd':
                case 'i':
                case 'u':
                    sputn(&obuf, value, 10, pad, padfill);
                    break;
                case 'o':
                    sputn(&obuf, value, 8, pad, padfill);
                    break;
                case 'p':
                case 'x':
                    sputn(&obuf, value, 16, pad, padfill);
                    break;
                /* Folgendes wieder einkommentieren um das extrem coole
                   Feature "rekursives printf" auszuprobieren */
                /*case 'r':
                    format2 = (char*)*(*args)++;
                    kaprintf(format2, args);
                    break;*/
                case 's':
                    sputs(&obuf, (char*)*(*args)++);
                    break;
                default:
                    sputc(&obuf, '%');
                    sputc(&obuf, *format);
                    break;
            }
            format++;
        }
        else
        {
            sputc(&obuf, *format++);
        }
    }

    sputc(&obuf, '\0');
}

/* printf fÃ¼r den Kernel. Nur fÃ¼r Testzwecke gedacht.
 UnterstÃ¼tzt %c, %d, %i, %o, %p, %u, %s, %x und %lld, %lli, %llo, %llu, %llx. */
int sprintf(char* str, const char *format, ...)
{
    int * args = ((int*)&format) + 1;
    do_sprintf(str, format, &args);

    // FIXME: Stringlaenge zurückgeben
    return 0;
}

int asprintf(char** strp, const char *fmt, ...)
{
    int * args_orig = ((int*)&fmt) + 1;
    int * my_args = ((int*)&fmt) + 1;
    int ** args = &my_args;

    const char* format = fmt;
    size_t size = 0;
   
    while(*format)
    {
        if(*format == '%')
        {
            format++;

            if(*format == '0')
            {
                format++;
            }

            while(*format >= '0' && *format <= '9')
            {
                *format++;
            }

            if(format[0] == 'l' && format[1] == 'l' && (format[2] == 'd' || format[2] == 'i' || format[2] == 'o' || format[2] == 'u' || format[2] == 'x'))
            {
                *args += 2;
                format += 2;
            }
            else if(format[0] == 'd' || format[0] == 'u')
            {
                (*args)++;
            }
            else if(format[0] == 'i' || format[0] == 'o' || format[0] == 'p' || format[0] == 'x')
            {
                (*args)++;
            }

            // TODO Genauere Schätzungen bei den Zahlen abhängig vom Typ
            switch(*format)
            {
                case 0:
                    goto size_estimation_end;                    
                case '%':
                    size++;
                    break;
                case 'c':
                    (*args)++;
                    size++;
                    break;
                case 'd':
                case 'i':
                case 'u':
                    size += 20;
                    break;
                case 'o':
                    size += 24;
                    break;
                case 'p':
                case 'x':
                    size += 16;
                    break;
                case 's':
                    size += strlen((char*)*(*args)++);
                    break;
                default:
                    size += 2;
                    break;
            }
            format++;
        }
        else
        {
            format++;
            size++;
        }
    }
size_estimation_end:

    *strp = malloc(size);
    if (!*strp) {
        *strp = NULL;
        return 0;
    }
    
    do_sprintf(*strp, fmt, &args_orig);

    // FIXME: Hier fehlt ein realloc, um auf die tatsächlich benötigte Größe zu schrumpen
    return 0;
}
