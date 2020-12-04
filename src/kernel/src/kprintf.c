/*
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
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
#include <stddef.h>
#include <lost/config.h>

#include "kprintf.h"
#include "console.h"
#include "lock.h"


static lock_t printf_lock;

/* Dividiert ein uint64 durch einen uint32 und gibt das Ergebnis zurück */
unsigned long long divmod(unsigned long long dividend, unsigned int divisor, unsigned int * remainder)
{
#if CONFIG_ARCH == ARCH_AMD64
    if (remainder) {
        *remainder = dividend % divisor;
    }
    return dividend / divisor;
#else
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

    if (remainder) {
        *remainder = rem;
    }

    return quotient;
#endif
}

/* Gibt eine unsigned 64-bit Zahl mit beliebiger Basis zwischen 2 und 36 aus */
/*static*/ void kputn(unsigned long long x, int radix, int pad, char padchar)
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
        con_putc_ansi(padchar);
    }

    con_puts(r + 1);
}

/* Hilfsfunktion für kprintf() */
void kaprintf(char* format, va_list args)
{
    int pad;
    char padfill;
    unsigned long long value = 0;

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

            if(format[0] == 'l' && format[1] == 'l' && (format[2] == 'd' || format[2] == 'i'))
            {
                signed long long tmp = va_arg(args, long long int);
                if (tmp < 0) {
                    con_putc_ansi('-');
                    pad--;
                    value = -tmp;
                } else {
                    value = tmp;
                }
                format += 2;
            }
            else if(format[0] == 'l' && format[1] == 'l' && (format[2] == 'o' || format[2] == 'u' || format[2] == 'x'))
            {
                value = va_arg(args, unsigned long long int);
                format += 2;
            }
            else if(format[0] == 'l' && (format[1] == 'd' || format[1] == 'i'))
            {
                signed long int tmp = va_arg(args, long int);
                if(tmp < 0) {
                    con_putc_ansi('-');
                    pad--;
                    value = -tmp;
                } else {
                    value = tmp;
                }
                format++;
            } else if(format[0] == 'z' && (format[1] == 'u' || format[1] == 'x')) {
                value = va_arg(args, size_t);
                format++;
            }
            else if(format[0] == 'l' && (format[1] == 'o' || format[1] == 'u' || format[1] == 'x'))
            {
                value = va_arg(args, unsigned long int);
                format++;
            }
            else if(format[0] == 'd' || format[0] == 'i')
            {
                int tmp = va_arg(args, int);
                if(tmp < 0) {
                    con_putc_ansi('-');
                    pad--;
                    value = -tmp;
                } else {
                    value = tmp;
                }
            } else if(format[0] == 'u' || format[0] == 'o' || format[0] == 'x') {
                value = va_arg(args, unsigned int);
            } else if(format[0] == 'p') {
                value = va_arg(args, uintptr_t);
            }

            switch(*format)
            {
                case 0:
                    return;
                case '%':
                    con_putc_ansi('%');
                    break;
                case 'c':
                    //con_putc_ansi(*(*args)++);
                    con_putc(va_arg(args, int));
                    break;
                case 'd':
                case 'i':
                case 'u':
                    kputn(value, 10, pad, padfill);
                    break;
                case 'o':
                    kputn(value, 8, pad, padfill);
                    break;
                case 'p':
                case 'x':
                    kputn(value, 16, pad, padfill);
                    break;
                /* Folgendes wieder einkommentieren um das extrem coole
                   Feature "rekursives printf" auszuprobieren */
                /*case 'r':
                    format2 = (char*)*(*args)++;
                    kaprintf(format2, args);
                    break;*/
                case 's':
                    //con_puts((char*)*(*args)++);
                    con_puts(va_arg(args, char*));
                    break;
                default:
                    con_putc_ansi('%');
                    con_putc_ansi(*format);
                    break;
            }
            format++;
        }
        else
        {
            con_putc_ansi(*format++);
        }
    }
    
    con_flush_ansi_escape_code_sequence();
}

/* printf fÃ¼r den Kernel. Nur fÃ¼r Testzwecke gedacht.
 UnterstÃ¼tzt %c, %d, %i, %o, %p, %u, %s, %x und %lld, %lli, %llo, %llu, %llx, %zu, %zx. */
void kprintf(char* format, ...)
{
    lock(&printf_lock);
    
    va_list args;
    
    va_start(args,format);
    kaprintf(format, args);
    va_end(args);

    unlock(&printf_lock);
}

int printf(const char* format, ...)
{
    lock(&printf_lock);
    
    va_list args;
    va_start(args,format);
    kaprintf((char*) format, args);
    va_end(args);
    
    unlock(&printf_lock);
    return 0; // TODO Korrekte printf-Rückgabe
}

