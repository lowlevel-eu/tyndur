/*
 * Copyright (c) 1999
 *	David E. O'Brien
 * Copyright (c) 1988, 1993
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
 * 3. Neither the name of the University nor the names of its contributors
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
 * Einen String aus breiten Zeichen kopieren, das ist das Pendant zu stpcpy. Die
 * beiden Strings duerfen sich nicht ueberlappen. Das abschliessende L'0' wird
 * mitkopiert.
 *
 * @see stpcpy
 * @see wcscpy
 * @see wcpncpy
 * @param dst Zeiger auf die Speicherstelle in die der Kopiertestring abgelegt
 *            werden soll. Dabei muss vom Aufrufer sichergestellt werden, dass
 *            dort mindestens speicher fuer wcslen(src) + 1 breite Zeichen ist.
 * @param src
 *
 * @return Zeiger auf das Ende (L'0') des Zielstrings.
 */
wchar_t* wcpcpy(wchar_t* dst, const wchar_t* src)
{
	for (; (*dst = *src); ++src, ++dst) { }

	return dst;
}

