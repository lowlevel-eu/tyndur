/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
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
 * String aus breiten Zeichen kopieren. Dabei werden maximal len Zeichen
 * kopiert. Die restlichen Zeichen im Puffer werden mit L'\0' gefuellt. Ist src
 * gleich lang wie oder laenger als len, wird dst nicht mit L'\0' terminiert.
 * Die beiden Strings duerfen sich nicht ueberlappen. Diese Funktion ist das
 * Pendant zu strncpy.
 *
 * @see strncpy
 * @see wcscpy
 * @param src String der kopiert werden soll
 * @param dst Zeiger auf den Speicherbereich in den src kopiert werden soll
 * @param len Anzahl der Zeichen, die maximal kopiert werden sollen
 *
 * @return dst
 */
wchar_t* wcsncpy(wchar_t* dst, const wchar_t* src, size_t len)
{
	if (len != 0) {
		wchar_t* d = dst;
		const wchar_t* s = src;

		do {
			if ((*d++ = *s++) == L'\0') {
				/* NUL pad the remaining n-1 bytes */
				while (--len != 0) {
					*d++ = L'\0';
                }
				break;
			}
		} while (--len != 0);
	}

	return dst;
}

