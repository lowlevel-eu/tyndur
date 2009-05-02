/*
 * Copyright (c) 2008-2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _WCHAR_H_
#define _WCHAR_H_

#include <stddef.h>
#include <stdio.h>

#define WEOF ((wint_t) -1)

typedef wchar_t wint_t;

/* WSTDIO */

/**
 * Breites Zeichen aus einer Datei lesen.
 *
 * @param stream Die geoeffnete Datei
 *
 * @return Das gelesene Zeichen oder WEOF im Fehlerfall.
 */
wint_t fgetwc(FILE* stream);

/**
 * Wie fgetwc, mit dem Unterschied, dass getwc als Makro implementiert werden
 * darf, das den Parameter mehrmals auswertet
 *
 * @see fgetwc
 */
#define getwc(stream) fgetwc(stream)

/**
 * Breites Zeichen von der Standardeingabe lesen.
 *
 * @see fgetwc
 */
wint_t getwchar(void);


/**
 * Ein breites Zeichen in eine Datei schreiben, dabei wird das Zeichen
 * entsprechend codiert.
 *
 * @param wc     Das zu schreibende Zeichen
 * @param stream Die geoeffnete Datei
 *
 * @return Das geschriebene Zeichen oder WEOF im Fehlerfall
 */
wint_t fputwc(wchar_t wc, FILE* stream);

/**
 * Wie fputwc, mit dem Unterschied, dass putwc als Makro implementiert werden
 * darf, das seine Parameter mehrmals auswertet.
 *
 * @see fputwc
 */
#define putwc(wc, stream) fputwc(wc, stream)

/**
 * Breites Zeichen auf die Standardausgabe schreiben
 *
 * @see fputwc
 * @param wc Das zu schreibende Zeichen
 *
 * @return Das geschriebene Zeichen oder WEOF im Fehlerfall
 */
wint_t putwchar(wchar_t wc);

#endif //ifndef _WCHAR_H_

