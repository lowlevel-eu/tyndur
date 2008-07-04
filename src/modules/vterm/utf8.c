/*
 * Copyright (c) 2007 The LOST Project. All rights reserved.
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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "vterm.h"

/**
 * Zeichentabelle um Unicode-Zeichen in CP437 zu ersetzen
 */
struct {
    wchar_t uc;
    char    cp;
} uc_to_cp [] = {
    {0x00E4, 0x84}, // ae
    {0x00F6, 0x94}, // oe
    {0x00FC, 0x81}, // ue
    {0x00C4, 0x8E}, // AE
    {0x00D6, 0x99}, // OE
    {0x00DC, 0x9A}, // UE
    {0x00DF, 0xE1}, // SS

    {0x00AB, 0xAE}, // <<
    {0x00BB, 0xAF}, // >>
};

/**
 * Unicode-Zeichen in der Tabelle nachschlagen und in cp437 umwandeln
 *
 * @param wc Unicode-Zeichen
 *
 * @return cp437-Zeichen oder 0 wenn keines gefunden wurde
 */
static char to_cp437(wchar_t wc)
{
    int i;

    // ASCII
    if (wc <= 0x7F) {
        return (char) wc;
    }

    // Tabelle durchsuchen
    for (i = 0; i < sizeof(uc_to_cp) / sizeof(uc_to_cp[0]); i++) {
        if (wc == uc_to_cp[i].uc) {
            return uc_to_cp[i].cp;
        }
    }
    return 0x00;
}

/**
 * Wandelt das Zeichen in cp437 um oder legt es in den utf8 Buffer, falls es
 * ein Teil eines Zeichens ist, das mehrere Bytes belegt
 *
 * @param c     Das Zeichen
 * @param dest  Pointer auf die Speicherstelle in der das cp437-Zeichen
 *              abgelegt wird
 *
 * @return TRUE wenn das Zeichen umgewandelt wurde, FALSE sonst
 */
static bool convert_char(vterminal_t* vterm, char c, char* dest)
{
    int len = vterm->utf8_buffer_offset;
    wchar_t wc;

    // Erstes Zeichen im Buffer abschneiden bei mehr als 4, danach das neue
    // anhaengen
    if (len >= 4) {
            len = 3;
            memmove(vterm->utf8_buffer, vterm->utf8_buffer + 1, len);
    }
    vterm->utf8_buffer[len++] = c;


    // Versuchen ein Zeichen zu konvertieren
    if (mbtowc(&wc, vterm->utf8_buffer, len) == -1) {
        // Wenn das nicht klappt, wird das ganze im Buffer gelassen
        vterm->utf8_buffer_offset = len;
        return FALSE;
    } else {
        *dest = to_cp437(wc);
        vterm->utf8_buffer_offset = 0;

        // Bei einem Zeichen, das nicht kopiert werden kann, wird ein ? als
        // Platzhalter geschrieben
        if (!*dest) {
            *dest = '?';
        }
        return TRUE;
    }

}

int utf8_to_cp437(vterminal_t* vterm, const char* str, size_t len, char* buf)
{
    int l = 0;
    int i;

    for (i = 0; i < len; i++) {
        if (convert_char(vterm, str[i], buf + l)) {
            l++;
        }
    }
    return l;
}

