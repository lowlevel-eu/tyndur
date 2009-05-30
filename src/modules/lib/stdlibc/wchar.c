/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
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

#include <wchar.h>
#include <stdlib.h>
#include <errno.h>

// Diese werden auch fuer wcstombs und mbstowcs benutzt
extern size_t __internal_wcsrtombs(char* buf, const wchar_t** wcs, size_t len);
extern size_t __internal_mbsrtowcs(wchar_t* buf, const char** str, size_t len);


/**
 * Reentrante Variante von mbtowc, bei uns mit UTF-8 aber identisch.
 * @see mbtowc
 */
size_t mbrtowc(wchar_t* wc, const char* s, size_t len, mbstate_t* ps)
{
    return mbtowc(wc, s, len);
}

/**
 * Reentrante Variante von wctomb, bei uns mit UTF-8 aber identisch.
 * @see wctomb
 */
size_t wcrtomb(char* buf, wchar_t wc, mbstate_t* ps)
{
    return wctomb(buf, wc);
}

/**
 * Reentrante Variante von wcstombs. Der einzige wesentliche Unterschied fuer
 * uns mit UTF8 ist, dass *wcs so aktualisiert wird, dass es bei einem Abbruch,
 * sei es weil buf zu klein ist oder weil ein ungueltiges Zeichen angetroffen
 * wurde, auf das betreffende Zeichen zeigt. Wird der String erfolgreich
 * verarbeitet, wird *wcs auf NULL gesetzt.
 * @see wcstombs
 */
size_t wcsrtombs(char* buf, const wchar_t** wcs, size_t len, mbstate_t* ps)
{
    size_t result;

    if ((result = __internal_wcsrtombs(buf, wcs, len)) == (size_t) -1) {
        errno = EILSEQ;
    }

    return result;
}

/**
 * Reentrante Variante von mbstowcs. Der einzige wesentliche Unterschied fuer
 * uns mit UTF8 ist, dass *str so aktualisiert wird, dass es bei einem Abbruch,
 * sei es weil buf zu klein ist oder weil ein ungueltiges Zeichen angetroffen
 * wurde, auf das betreffende Zeichen zeigt. Wird der String erfolgreich
 * verarbeitet, wird *str auf NULL gesetzt.
 * @see mbstowcs
 */
size_t mbsrtowcs(wchar_t* buf, const char** str, size_t len, mbstate_t* ps)
{
    size_t result;

    if ((result = __internal_mbsrtowcs(buf, str, len)) == (size_t) -1) {
        errno = EILSEQ;
    }

    return result;
}


/**
 * Anzahl der Spalten, die ein Zeichen in Anspruch nimmt, errechnen. Fuer c = 0
 * wird 0 zuruekgegeben. Falls es sich nicht um ein druckbares Zeichen handelt,
 * wird -1 zurueckgegeben.
 *
 * @param wc Das breite Zeichen
 *
 * @return Anzahl Spalten, oder -1 wenn das Zeichen nicht druckbar ist.
 */
int wcwidth(wchar_t wc)
{
    // Muss nach Spezifikation so sein
    if (!wc) {
        return 0;
    }

    // Steuerzeichen sind nicht druckbar
    if (wc < 32) {
        return -1;
    }

    // Das ist so sicher noch etwas rudimentaer, doch im Moment reicht das so
    // fuer uns.
    return 1;
}

/**
 * Anzahl der Spalten, die ein String aus breiten Zeichen in Anspruch nimmt,
 * errechnen. Falls beim verarbeiten des Strings ein L'\0' angetroffen wird,
 * wird die Verarbeitung des Strings abgeschlossen, auch wenn noch nicht len
 * Zeichen verarbeitet wurden. Wird ein nicht druckbares Zeichen erkannt, wird
 * abgebrochen.
 *
 * @see wcwidth
 * @param wcs Zeiger auf das erste Zeichen
 * @param len Anzahl der Zeichen
 *
 * @return Anzahl der Zeichen oder -1 im Fehlerfall.
 */
int wcswidth(const wchar_t* wcs, size_t len)
{
    int cols = 0;
    int cur;
    size_t i;

    for (i = 0; (i < len) && (wcs[i] != L'\0'); i++) {
        if ((cur = wcwidth(wcs[i])) == -1) {
            return -1;
        }
        cols += cur;
    }

    return cols;
}

