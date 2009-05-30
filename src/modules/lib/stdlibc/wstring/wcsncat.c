/*-
 * Copyright (c)1999 Citrus Project,
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	citrus Id: wcsncat.c,v 1.1 1999/12/29 21:47:45 tshiozak Exp
 */

#include <wchar.h>


/**
 * Zwei Strings aus breiten Zeichen aneinanderhaengen. Dabei werden maximal len
 * Zeichen aus src kopiert. Der Aufrufer muss sicherstellen, dass nach dem
 * String, auf den dst zeigt, noch mindestens len + 1 Zeichen platz haben. dst
 * wird in jedem Fall mit L'\0' terminiert. Diese Funktion ist das Pendant zu
 * strncat.
 *
 * @see strncat
 * @see wcscat
 * @param dst String an den src angehaengt werden soll
 * @param src String der an dst angehaengt werden soll
 * @param len Anzahl der Zeichen, die maximal kopiert werden aus src
 *
 * @return dst
 */
wchar_t* wcsncat(wchar_t* dst, const wchar_t* src, size_t len)
{
	wchar_t* p;
	wchar_t* q;
	const wchar_t* r;

	p = dst;
	while (*p) {
		p++;
    }

	q = p;
	r = src;
	while (*r && len) {
		*q++ = *r++;
		len--;
	}
	*q = '\0';

	return dst;
}

