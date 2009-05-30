/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <wchar.h>


/**
 * Zwei Strings aus breiten Zeichen vergleichen. Dabei werden maximal len
 * Zeichen verglichen. Diese Funktion ist das Pendant zu strncmp.
 *
 * @see strncmp
 * @see wcscmp
 * @param wcs1 Erster String
 * @param wcs2 Zweiter String
 * @param len  Anzahl der Zeichen, die maximal verglichen werden sollen, wenn
 *             vorher kein abschliessendes L'\0' angetroffen wurde.
 *
 * @return 0 wenn die Strings gleich sind, oder <0 wenn das erste
 *         ungleiche Zeichen in wcs1 kleiner ist als das in wcs2, ist es
 *         groesser >0.
 */
int wcsncmp(const wchar_t* wcs1, const wchar_t* wcs2, size_t len)
{

	if (len == 0) {
		return 0;
    }

	do {
		if (*wcs1 != *wcs2++) {
			/* XXX assumes wchar_t = int */
			return (*(unsigned int*)wcs1 - *(unsigned int*)--wcs2);
		}

		if (*wcs1++ == 0) {
			break;
        }
	} while (--len != 0);
	return 0;
}

