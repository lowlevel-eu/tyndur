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
#include <lost/config.h>
#include <string.h>
#include <stdlib.h>

size_t strnlen(const char *s, size_t maxlen)
{
    size_t len = 0;
    
    while (*s++ && maxlen--) {
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
unsigned int atoi(const char *s)
{
	size_t digit = 0;
	size_t highest = 0;
	size_t i;
    for (i=0;s[i] != '\0';i++)
	{
		if (s[i] < '0' ||
			s[i] > '9')break;
		if (highest == 0)highest = 1;
		else highest *= 10;
		digit++;
	}
	unsigned int result = 0;
	for (i=0;i < digit;i++)
	{
		result += (s[i] - '0') * highest;
		highest /= 10;
	}
	return result;
}
//

long atol(const char *str)
{
    if (!*str) {
        return 0;
    }
    int positive = 1;
    long result = 0;
    int length = 0;
    //Vorzeichen
    if (*str == '-') {
        positive = 0;
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
            if (!length) {
                return 0;
            }
            return result;
        } 
        str++;
    }
            
    if (!length) {
        return 0;
    }
    return result;
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

