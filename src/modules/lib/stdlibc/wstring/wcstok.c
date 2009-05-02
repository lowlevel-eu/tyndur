/*-
 * Copyright (c) 1998 Softweyr LLC.  All rights reserved.
 *
 * strtok_r, from Berkeley strtok
 * Oct 13, 1998 by Wes Peters <wes@softweyr.com>
 *
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notices, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notices, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SOFTWEYR LLC, THE REGENTS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL SOFTWEYR LLC, THE
 * REGENTS, OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <wchar.h>


/**
 * String aus breiten Zeichen in Tokens aufspalten, die durch die in delim
 * angegebenen Zeichen getrennt werden. wcs wird dabei veraendert. Ist wcs !=
 * NULL beginnt die suche dort, sonst wird bei *last begonnen. *last wird
 * jeweils auf den Anfang des naechsten Token gesetzt, oder auf NULL, wenn das
 * Ende erreicht wurde.
 * Diese Funktion ist das Pendant zu strtok.
 *
 * @see strtok
 * @param wcs   Zeiger auf das Zeichen bei dem die Zerlegung in Tokens begonnen
 *              werden soll, oder NULL, wenn der Wert von *last genommen werden
 *              soll.
 * @param delim Zeichen die zwei Tokens voneinander trennen koennen
 * @param last  Zeiger auf die Speicherstelle an der die Funktion die Position
 *              des naechsten token speichern kann fuer den internen Gebrauch,
 *              um beim Naechsten Aufruf mit wcs == NULL das naechste Token
 *              zurueck geben zu koennen.
 *
 * @return Zeiger auf das aktuelle Token, oder NULL wenn keine Tokens mehr
 *         vorhanden sind.
 */
wchar_t* wcstok(wchar_t* wcs, const wchar_t* delim, wchar_t** last)
{
	const wchar_t* spanp;
	wchar_t* tok;
	wchar_t c, sc;

	if (wcs == NULL && (wcs = *last) == NULL) {
		return NULL;
    }

	/*
	 * Skip (span) leading delimiters (wcs += wcsspn(wcs, delim), sort of).
	 */
cont:
	c = *wcs++;
	for (spanp = delim; (sc = *spanp++) != L'\0';) {
		if (c == sc) {
			goto cont;
        }
	}

	if (c == L'\0') {	/* no non-delimiter characters */
		*last = NULL;
		return NULL;
	}
	tok = wcs - 1;

	/*
	 * Scan token (scan for delimiters: s += wcscspn(s, delim), sort of).
	 * Note that delim must have one NUL; we stop if we see that, too.
	 */
	for (;;) {
		c = *wcs++;
		spanp = delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == L'\0') {
					wcs = NULL;
				} else {
					wcs[-1] = L'\0';
                }
				*last = wcs;

				return tok;
			}
		} while (sc != L'\0');
	}
	/* NOTREACHED */
}

