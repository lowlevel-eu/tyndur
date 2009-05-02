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

/**
 * String aus breiten Zeichen kopieren. Dabei werden hoechstens len Zeichen
 * kopiert. Sind in src weniger als len Zeichen wird dst mit L'\0' aufgefuellt.
 * Ist src laenger als oder gleich lang wie len wird der String in dst nich
 * nullterminiert, der Rueckgabewert zeigt also nich auf ein L'\0'. Die beiden
 * Speicherbereiche duerfen sich nicht ueberlappen. Diese Funktion ist das
 * Pendant zu strncpy.
 *
 * @see strncpy
 * @see wcpncpy
 * @param dst Zeiger auf den Speicherbereich der len breite Zeichen aufnehmen
 *            kann. Es werden genau len Zeichen hineingeschrieben.
 * @param src Quellstring
 * @param len Anzahl der Zeichen, die in dst geschrieben werden sollen.
 *
 * @return Zeiger auf das letzte geschriebene Byte, also immer dst + len - 1
 */
wchar_t* wcpncpy(wchar_t* dst, const wchar_t* src, size_t len)
{
	for (; len--; dst++, src++) {
		if (!(*dst = *src)) {
			wchar_t *ret = dst;
			while (len--) {
				*++dst = L'\0';
            }
			return ret;
		}
	}

	return dst;
}

