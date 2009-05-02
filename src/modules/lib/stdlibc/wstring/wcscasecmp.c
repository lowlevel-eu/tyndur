/*-
 * Copyright (c) 2009 David Schultz <das@FreeBSD.org>
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
 */

#include <wchar.h>
#include <wctype.h>

/**
 * Zwei Strings aus breiten Zeichen ohne Unterscheidung von
 * Gross-/Kleinschreibung vergleichen inklusive abschliessendes L'\0'. Diese
 * Funktion ist das Pendant zu strcasecmp.
 *
 * @see strcasecmp
 * @see wcscmp
 * @see towlower
 * @param wcs1 Erster String
 * @param wcs2 Zweiter String
 *
 * @return 0 wenn die Strings gleich sind, oder <0 wenn das erste
 *         ungleiche Zeichen in wcs1 kleiner ist als das in wcs2, ist es
 *         groesser >0.
 */
int wcscasecmp(const wchar_t* wcs1, const wchar_t* wcs2)
{
	wchar_t wc1, wc2;

	for (; *wcs1; wcs1++, wcs2++) {
		wc1 = towlower(*wcs1);
		wc2 = towlower(*wcs2);
		if (wc1 != wc2) {
			return (int) (wc1 - wc2);
        }
	}

	return -*wcs2;
}

