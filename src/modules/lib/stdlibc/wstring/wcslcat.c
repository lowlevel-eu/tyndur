/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	from OpenBSD: strlcat.c,v 1.3 2000/11/24 11:10:02 itojun Exp
 */

#include <wchar.h>


/**
 * Kopiert src an das Ende des Strings dst. Dabei wird der String dst auf
 * maximal len - 1 Zeichen und L'\0' verlaengert, ausser wenn len == 0.
 *
 * @see wcsncat
 * @param dst Zeiger auf String an den src angehaengt werden soll
 * @param src Zeiger auf String der an dst angehaengt werden soll
 * @param len Anzahl Zeichen auf die dst Maximal verlaengert werden darf
 *            inklusiv L'\0.'
 *
 * @return wcslen(urspruengliches dst) + wcslen(src); Wenn der Rueckgabewert >=
 *         len ist, wurde src nich vollstaendig kopiert.
 */
size_t wcslcat(wchar_t* dst, const wchar_t* src, size_t len)
{
	wchar_t* d = dst;
	const wchar_t* s = src;
	size_t n = len;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (*d != L'\0' && n-- != 0) {
		d++;
    }
	dlen = d - dst;
	n = len - dlen;

	if (n == 0) {
		return(dlen + wcslen(s));
    }
	while (*s != L'\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = L'\0';

	return(dlen + (s - src));	/* count does not include NUL */
}

