/*
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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

#include "syscall.h"
#include "stdio.h"
#include "string.h"

FILE* stdin = NULL;
FILE* stdout = NULL;
FILE* stderr = NULL;

//Nur fuer Funktionen wie printf
int putsn(unsigned int n, const char * str)
{
    if(stdout == NULL)
    {
        syscall_putsn(n, str);
        return n;
    }
    else
    {
        if(fwrite(str, n, 1, stdout) == EOF)
        {
            return EOF;
        }
        else
        {
            return n;
        }
    }
}

int puts(const char* str)
{
    if(putsn(strlen(str), str) == EOF)
    {
        return EOF;
    }
    else
    {
        if(stdout == NULL)
        {
            syscall_putsn(1, "\n");
        }
        else
        {
            putchar('\n');
        }
    }
    return 1;
}

int putc(int c, FILE *stream)
{
    if(stdout == NULL)
    {
        char cs = c;
        syscall_putsn(1, &cs);
    }
    else
    {
        return fputc(c, stream);
    }
    return c;
}

int putchar(int c)
{
    if(stdout == NULL)
    {
        char cs = c;
        syscall_putsn(1, &cs);
    }
    else
    {
        return fputc(c, stdout);
    }
    return c;
}
