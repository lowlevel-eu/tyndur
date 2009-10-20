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

#include <stdlib.h>
#include <string.h>


/**
 * UTF8:
 *
 * x steht fuer ein beliebiges Bit
 * Laenge   Kodierung
 * 1 Byte   0xxxxxxx
 * 2 Byte   110xxxxx 10xxxxxx
 * 3 Byte   1110xxxx 10xxxxxx 10xxxxxx
 * 4 Byte   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 */
#define BIT(b) (1 << (b))

// Bitmaske zum Feststellen der Laenge anhand des ersten Zeichens
#define MSK_1 (BIT(7))
#define MSK_2 (BIT(7) | BIT(6) | BIT(5))
#define MSK_3 (BIT(7) | BIT(6) | BIT(5) | BIT(4))
#define MSK_4 (BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3))

// Werte der hoechsten Bits im ersten Byte um die Laenge festzustellen
#define VAL_1 0
#define VAL_2 (BIT(7) | BIT(6))
#define VAL_3 (BIT(7) | BIT(6) | BIT(5))
#define VAL_4 (BIT(7) | BIT(6) | BIT(5) | BIT(4))

#define get_lower_bits(v, x) (v & (BIT(x) - 1))


/**
 * Anzahl der Bytes die das erste Zeichen belegt. Handelt es sich um ein L'\0',
 * ist die Laenge 0.
 *
 * @param s     Pointer auf den Anfang den Anfang des Zeichens
 * @param slen  Maximale Laenge die das Zeichen haben kann (Stringlaenge)
 *
 * @return Laenge des Zeichens oder -1 wenn ein Fehler auftritt (z.B.
 *         ungueltiges Zeichen)
 */
int mblen(const char* s, size_t slen)
{
    int len = 0;
    int i;

    // Laenge anhand des ersten Zeichens bestimmen
    if ((s[0] & MSK_4) == VAL_4) {
        len = 4;
    } else if ((s[0] & MSK_3) == VAL_3) {
        len = 3;
    } else if ((s[0] & MSK_2) == VAL_2) {
        len = 2;
    } else if ((s[0] & MSK_1) == VAL_1) {
        // Bei einem L'\0' soll 0 zurueckgegeben werden
        if (!s[0]) {
            len = 0;
        } else {
            len = 1;
        }
    } else {
        return -1;
    }

    // Der String enthaelt nicht das ganze Zeichen
    if (slen < len) {
        return -1;
    }

    // Pruefen ob die Folgebytes alle mit 10 beginnen
    for (i = 1; i < len; i++) {
        if ((s[i] & (BIT(7) | BIT(6))) != BIT(7)) {
            return -1;
        }
    }

    return len;
}


/**
 * Erstes Zeichen im String in einen wchar umwandeln. Wird NULL als wc
 * uebergeben, gibt die Funktion lediglich die Laenge des Zeichens zurueck
 * (mblen). Handelt es sich beim ersten Zeichen um ein L'\0', soll 0 als Laenge
 * zurueckgegeben werden. Ist s NULL gibt die Funktion 0 zurueck.
 *
 * @param wc    Pointer auf den wchar in dem das Ergebnis abgelegt werden soll
 * @param s     Pointer auf den Anfang des Zeichens
 * @param len   Maximale Laenge die das Zeichen haben kann (Stringlaenge)
 *
 * @return Bei Erfolg wird die Anzahl der benutzten Bytes aus s zurueckgegeben,
 *         im Fehlerfall -1
 */
int mbtowc(wchar_t* wc, const char* s, size_t slen)
{
    int len;
    int i;
    int bitpos = 0;

    if (s == NULL) {
        return 0;
    }

    len = mblen(s, slen);
    if (wc == NULL) {
        return len;
    }

    if (len == -1) {
        return -1;
    }

    // Erstes Zeichen wird separat behandelt
    bitpos = 7 - (len - 1);
    *wc = get_lower_bits(s[0], bitpos);

    // Die anderen Zeichen sind alle gleich
    for (i = 1; i < len; i++) {
        *wc = *wc << 6;
        *wc |= get_lower_bits(s[i], 6);
    }

    return len;
}

/**
 * Einen wchar in ein Multibyte-Zeichen umwandeln. Der Aufrufer muss
 * garantieren dass in buf mindestens MB_CUR_MAX Bytes verfuegbar sind.
 *
 * Falls buf == NULL uebergeben wird, soll der interne "shift state"
 * zurueckgesetzt werden und != 0 zurueckgegeben werden, falls die aktuelle
 * Kodierung sowas hat. Solange wir nur UTF-8 benutzen brauchen wir uns aber
 * darum nicht zu kuemmern.
 *
 * @param buf Puffer in dem das Multibyte-Zeichen abgelegt wird. Dabei koennen
 *            bis zu MB_CUR_MAX Bytes geschrieben werden.
 * @param wc  Der umzuwandelnde wchar
 *
 * @return Bei Erfolg wir die Anzahl der in buf geschrieben Bytes
 *         zurueckgegeben, im Fehlerfall -1. Falls buf == NULL war, wird 0
 *         zurueckgegeben.
 */
int wctomb(char* buf, wchar_t wc)
{
    if (buf == NULL) {
        return 0;
    }

    if (get_lower_bits(wc, 7) == wc) {
        buf[0] = get_lower_bits(wc, 7) | VAL_1;
        return 1;
    } else if (get_lower_bits(wc, 11) == wc) {
        buf[0] = get_lower_bits((wc >> 6), 5) | VAL_2;
        buf[1] = get_lower_bits((wc >> 0), 6) | BIT(7);
        return 2;
    } else if (get_lower_bits(wc, 16) == wc) {
        buf[0] = get_lower_bits((wc >> 12), 4) | VAL_3;
        buf[1] = get_lower_bits((wc >>  6), 6) | BIT(7);
        buf[2] = get_lower_bits((wc >>  0), 6) | BIT(7);
        return 3;
    } else {
        buf[0] = get_lower_bits((wc >> 18), 3) | VAL_4;
        buf[1] = get_lower_bits((wc >> 12), 6) | BIT(7);
        buf[2] = get_lower_bits((wc >>  6), 6) | BIT(7);
        buf[3] = get_lower_bits((wc >>  0), 6) | BIT(7);
        return 4;
    }
}

/**
 * Wird sowohl von wcstombs als auch von wcsrtombs benutzt, und entspricht
 * abgesehen vom wcstate-Kram wcsrtombs.
 *
 * @see wcstombs
 * @see wcsrtombs
 */
size_t __internal_wcsrtombs(char* buf, const wchar_t** wcs, size_t len)
{
    char intbuf[MB_CUR_MAX];
    size_t bufpos = 0;
    int curlen;
    const wchar_t* our_wcs;

    if (!buf) {
        // Wenn buf == NULL ist, duerfen wir auch *wcs nicht veraendern, deshalb
        // setzen wir wcs auf den lokalen Pointer
        our_wcs = *wcs;
        wcs = &our_wcs;
    }

    while (**wcs && (!buf || (bufpos < len))) {
        if ((curlen = wctomb(intbuf, *(*wcs)++)) == -1) {
            return (size_t) -1;
        }

        if (buf && ((len - bufpos) < curlen)) {
            // Schade, zu eng, das Zeichen passt nicht mehr vollstaendig in den
            // Puffer
            break;
        }

        // Passt noch rein
        if (buf) {
            memcpy(buf + bufpos, intbuf, curlen);
        }
        bufpos += curlen;
    }

    // Wenn wir am Ende angekommen sind, und noch Platz frei ist, terminieren
    // wir das ganze noch mit einem netten kleinen '\0'.
    if (buf && !**wcs && (bufpos < len)) {
        *wcs = NULL;
        buf[bufpos] = '\0';
    }

    return bufpos;
}

/**
 * Wird sowohl von mbstowcs als auch von mbsrtowc benutzt, und entspricht
 * abgesehen vom wcstate-Kram mbstowcs.
 *
 * @see mbstowcs
 * @see mbsrtowcs
 */
size_t __internal_mbsrtowcs(wchar_t* buf, const char** str, size_t len)
{
    wchar_t wc;
    size_t bufpos = 0;
    int curlen;
    const char* our_str;

    if (!buf) {
        // Wenn buf == NULL ist, duerfen wir auch *str nicht veraendern, deshalb
        // setzen wir str auf den lokalen Pointer
        our_str = *str;
        str = &our_str;
    }

    while (**str && (!buf || (bufpos < len))) {
        if ((curlen = mbtowc(&wc, *str, MB_CUR_MAX)) == -1) {
            // Ein ungueltiges Zeichen wurde angetroffen
            return (size_t) -1;
        }

        if (buf) {
            buf[bufpos] = wc;
        }

        bufpos++;
        *str += curlen;
    }

    // Wenn noch Platz ist, schreiben wir jetzt das abschliessende L'\0'
    if (buf && !**str && (bufpos < len)) {
        *str = NULL;
        buf[bufpos] = L'\0';
    }

    return bufpos;
}

/**
 * String aus breiten Zeichen in Multibyte-String umwandeln. Wird ein Zeichen
 * angetroffen, das nicht umgewandeld werden kann, wird -1 zurueckgegeben. Es
 * werden maximal len Bytes geschrieben. Das abschliessende L'\0' wird
 * mitkonvertiert, aber beim Rueckgabewert, wird es nicht mit beruecksichtigt.
 * Falls der String nicht vollstaendig konvertiert werden konnte, weil buf zu
 * klein ist, ist das Ergebnis nicht nullterminiert.
 *
 * Ist buf == NULL wird len ignoriert und es wird nur die Laenge bestimmt, die
 * der String haette, ohne das abschliessende 0-Byte.
 *
 * Um einen String vollstaendig zu konvertieren, muss buf mindestens die Groesse
 * wcstombs(NULL, wcs, 0) + 1 haben.
 *
 * @param buf Puffer in dem der Multibytestring abgelegt werden soll oder NULL
 * @param wcs Zeiger auf den String aus breiten Zeichen
 * @param len Groesse von buf
 *
 * @return Anzahl der in buf geschriebenen Bytes ohne abschliessendes '\0', oder
 *         (size_t) -1 wenn ein Zeichen nicht konvertiert werden konnte.
 */
size_t wcstombs(char* buf, const wchar_t* wcs, size_t len)
{
    return __internal_wcsrtombs(buf, &wcs, len);
}

/**
 * Multibyte-String in String aus breiten Zeichen umwandeln. Wird ein Zeichen
 * angetroffen, das nicht umgewandeld werden kann, wird -1 zurueckgegeben. Es
 * werden maximal len breite Zeichen geschrieben. Das abschliessende '\0' wird
 * mitkonvertiert, aber beim Rueckgabewert, wird es nicht mit beruecksichtigt.
 * Falls der String nicht vollstaendig konvertiert werden konnte, weil buf zu
 * klein ist, ist das Ergebnis nicht L'\0'-terminiert.
 *
 * Ist buf == NULL wird len ignoriert und es wird nur die Laenge bestimmt, die
 * der String haette, ohne das abschliessende L'\0'.
 *
 * Um einen String vollstaendig zu konvertieren, muss buf mindestens die Groesse
 * von mbstowcs(NULL, str, 0) + 1 breiten Zeichen haben.
 *
 * @param buf Puffer in dem der String aus breiten Zeichen abgelegt werden soll
 *            oder NULL
 * @param wcs Zeiger auf den Multibytestring
 * @param len Groesse von buf in Zeichen
 *
 * @return Anzahl der in buf geschriebenen Zeichen ohne abschliessendes L'\0',
 *          oder (size_t) -1 wenn ein Zeichen nicht konvertiert werden konnte.
 */
size_t mbstowcs(wchar_t* buf, const char* str, size_t len)
{
    return __internal_mbsrtowcs(buf, &str, len);
}

