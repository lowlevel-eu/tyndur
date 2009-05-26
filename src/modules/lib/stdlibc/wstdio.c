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

/**
 * Breites Zeichen aus einer Datei lesen.
 *
 * @param stream Die geoeffnete Datei
 *
 * @return Das gelesene Zeichen oder WEOF im Fehlerfall.
 */
wint_t fgetwc(FILE* stream)
{
    char mbc[MB_CUR_MAX];
    int len = 0;
    int mbres;
    int c;
    wint_t result;

    do {
        if ((c = fgetc(stream)) == EOF) {
            return WEOF;
        }

        mbc[len++] = c;
    } while (((mbres = mbtowc(&result, mbc, len)) == -1) && (len < MB_CUR_MAX));

    // Das Zeichen konnte nicht konvertiert werden. Wir schicken alle bytes bis
    // aufs erste mittels ungetc zurueck
    if (mbres < 0) {
        int i;
        for (i = len - 1; i > 0; i--) {
            ungetc(mbc[i], stream);
        }

        errno = EILSEQ;
        return WEOF;
    }

    return result;
}

/**
 * Breites Zeichen von der Standardeingabe lesen.
 *
 * @see fgetwc
 */
wint_t getwchar()
{
    return fgetwc(stdin);
}


/**
 * Ein breites Zeichen in eine Datei schreiben, dabei wird das Zeichen
 * entsprechend codiert.
 *
 * @param wc     Das zu schreibende Zeichen
 * @param stream Die geoeffnete Datei
 *
 * @return Das geschriebene Zeichen oder WEOF im Fehlerfall
 */
wint_t fputwc(wchar_t wc, FILE* stream)
{
    char mbc[MB_CUR_MAX];
    int len;

    if ((len = wctomb(mbc, wc)) < 0) {
        errno = EILSEQ;
        return WEOF;
    }

    if (!fwrite(mbc, len, 1, stream)) {
        return WEOF;
    }

    return wc;
}

/**
 * Breites Zeichen auf die Standardausgabe schreiben
 *
 * @see fputwc
 * @param wc Das zu schreibende Zeichen
 *
 * @return Das geschriebene Zeichen oder WEOF im Fehlerfall
 */
wint_t putwchar(wchar_t wc)
{
    return fputwc(wc, stdout);
}

/**
 * Einen String aus breiten Zeichen in den angegebenen Stream schreiben. Das
 * abschliessende L'\0' wird nicht mitgeschrieben.
 *
 * @param wcs    Zeiger auf den String aus breiten Zeichen
 * @param stream Die geoeffnete Datei
 *
 * @return Bei Erfolg > 0, im Fehlerfall -1
 */
int fputws(const wchar_t* wcs, FILE* stream)
{
    while (*wcs) {
        if (fputwc(*wcs, stream) == WEOF) {
            return -1;
        }
        wcs++;
    }
    return 0;
}

