/*
 * Copyright (c) 2008 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#define VAL_4 (BIT(7) | BIT(6) | BIT(5) | BIT(6))

#define get_lower_bits(v, x) (v & (BIT(x + 1) - 1))


/**
 * Anzahl der Bytes die das erste Zeichen belegt
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
        len = 1;
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
 * (mblen). Ist s NULL gibt die Funktion 0 zurueck.
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
    bitpos = (len == 1 ? 7 : 7 - len);
    *wc = get_lower_bits(s[0], bitpos);

    // Die anderen Zeichen sind alle gleich
    for (i = 1; i < len; i++) {
        *wc = *wc << 6;
        *wc |= get_lower_bits(s[i], 6);
    }

    return len;
}

