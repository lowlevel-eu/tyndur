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
 *	citrus Id: wmemcmp.c,v 1.2 2000/12/20 14:08:31 itojun Exp
 */

#include <wchar.h>


/**
 * Zwei Speicherbereiche aus breiten Zeichen vergleichen. Diese Funktion ist das
 * Pendant zu memcmp.
 *
 * @see memcmp
 * @see wcscpm
 * @see wcsncmp
 * @param wcs1 Zeiger auf den ersten Speicherbereich
 * @param wcs2 Zeiger auf den zweiten Speicherbereich
 * @param len  Laenge der beiden Speicherbereiche
 *
 * @return 0 wenn die beiden Speicherbereiche gleich sind, < 0 wenn das erste
 *         unterschiedliche Zeichen in wcs1 kleiner ist als das
 *         Korrespondierende in wcs2 oder > 0 wenn es groesser ist.
 */
int wmemcmp(const wchar_t* wcs1, const wchar_t* wcs2, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		if (*wcs1 != *wcs2) {
			/* wchar might be unsigned */
			return *wcs1 > *wcs2 ? 1 : -1;
		}

		wcs1++;
		wcs2++;
	}
	return 0;
}

