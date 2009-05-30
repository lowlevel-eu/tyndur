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
 *	citrus Id: wcscat.c,v 1.1 1999/12/29 21:47:45 tshiozak Exp
 */

#include <wchar.h>


/**
 * Zwei Strings aus breiten Zeichen aneinanderhaengen. Dabei wird der Inhalt von
 * src inklusive dem abschliessenden L'\0' ans Ende von dst kopiert. Der
 * Aufrufer muss sicherstellen, dass nach dst genug Speicher frei ist. Diese
 * Funktion ist das Pendant zu strcat.
 *
 * @see strcat
 * @see wcsncat
 * @param dst Zeiger auf den String an dessen Ende src angehaengt werden soll.
 * @param src Zeiger auf den String der an dst angehaengt werden soll.
 *
 * @return dst
 */
wchar_t* wcscat(wchar_t* dst, const wchar_t* src)
{
	wchar_t* wcp;

	wcp = dst;
	while (*wcp != L'\0') {
		wcp++;
    }
	while ((*wcp++ = *src++) != L'\0') { }

	return dst;
}

