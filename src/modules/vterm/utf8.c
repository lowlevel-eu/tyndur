/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
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

#include <stdbool.h>
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
    {0x0000A0, 0xFF}, // NO-BREAK SPACE
    {0x0000A1, 0xAD}, // INVERTED EXCLAMATION MARK
    {0x0000A2, 0x9B}, // CENT SIGN
    {0x0000A3, 0x9C}, // POUND SIGN
    {0x0000A5, 0x9D}, // YEN SIGN
    {0x0000A7, 0x15}, // SECTION SIGN
    {0x0000AA, 0xA6}, // FEMININE ORDINAL INDICATOR
    {0x0000AB, 0xAE}, // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK (<<)
    {0x0000AC, 0xAA}, // NOT SIGN
    {0x0000B0, 0xF8}, // DEGREE SIGN
    {0x0000B1, 0xF1}, // PLUS-MINUS SIGN
    {0x0000B2, 0xFD}, // SUPERSCRIPT TWO (^2)
    {0x0000B5, 0xE6}, // MICRO SIGN
    {0x0000B7, 0xFA}, // MIDDLE DOT
    {0x0000BA, 0xA7}, // MASCULINE ORDINAL INDICATOR
    {0x0000BB, 0xAF}, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK (>>)
    {0x0000BC, 0xAC}, // VULGAR FRACTION ONE QUARTER (1/4)
    {0x0000BD, 0xAB}, // VULGAR FRACTION ONE HALF (1/2)
    {0x0000BF, 0xA8}, // INVERTED QUESTION MARK
    {0x0000C4, 0x8E}, // LATIN CAPITAL LETTER A WITH DIAERESIS (Ae)
    {0x0000C5, 0x8F}, // LATIN CAPITAL LETTER A WITH RING ABOVE
    {0x0000C6, 0x92}, // LATIN CAPITAL LETTER AE (Ligatur AE)
    {0x0000C7, 0x80}, // LATIN CAPITAL LETTER C WITH CEDILLA
    {0x0000C9, 0x90}, // LATIN CAPITAL LETTER E WITH ACUTE
    {0x0000D1, 0xA5}, // LATIN CAPITAL LETTER N WITH TILDE
    {0x0000D6, 0x99}, // LATIN CAPITAL LETTER O WITH DIAERESIS (Oe)
    {0x0000DC, 0x9A}, // LATIN CAPITAL LETTER U WITH DIAERESIS (Ue)
    {0x0000DF, 0xE1}, // LATIN SMALL LETTER SHARP S (Eszett)
    {0x0000E0, 0x85}, // LATIN SMALL LETTER A WITH GRAVE
    {0x0000E1, 0xA0}, // LATIN SMALL LETTER A WITH ACUTE
    {0x0000E2, 0x83}, // LATIN SMALL LETTER A WITH CIRCUMFLEX
    {0x0000E4, 0x84}, // LATIN SMALL LETTER A WITH DIAERESIS (ae)
    {0x0000E5, 0x86}, // LATIN SMALL LETTER A WITH RING ABOVE
    {0x0000E6, 0x91}, // LATIN SMALL LETTER AE (Ligatur ae)
    {0x0000E7, 0x87}, // LATIN SMALL LETTER C WITH CEDILLA
    {0x0000E8, 0x8A}, // LATIN SMALL LETTER E WITH GRAVE
    {0x0000E9, 0x82}, // LATIN SMALL LETTER E WITH ACUTE
    {0x0000EA, 0x88}, // LATIN SMALL LETTER E WITH CIRCUMFLEX
    {0x0000EB, 0x89}, // LATIN SMALL LETTER E WITH DIAERESIS
    {0x0000EC, 0x8D}, // LATIN SMALL LETTER I WITH GRAVE
    {0x0000ED, 0xA1}, // LATIN SMALL LETTER I WITH ACUTE
    {0x0000EE, 0x8C}, // LATIN SMALL LETTER I WITH CIRCUMFLEX
    {0x0000EF, 0x8B}, // LATIN SMALL LETTER I WITH DIAERESIS
    {0x0000F1, 0xA4}, // LATIN SMALL LETTER N WITH TILDE
    {0x0000F2, 0x95}, // LATIN SMALL LETTER O WITH GRAVE
    {0x0000F3, 0xA2}, // LATIN SMALL LETTER O WITH ACUTE
    {0x0000F4, 0x93}, // LATIN SMALL LETTER O WITH CIRCUMFLEX
    {0x0000F6, 0x94}, // LATIN SMALL LETTER O WITH DIAERESIS (oe)
    {0x0000F7, 0xF6}, // DIVISION SIGN
    {0x0000F9, 0x97}, // LATIN SMALL LETTER U WITH GRAVE
    {0x0000FA, 0xA3}, // LATIN SMALL LETTER U WITH ACUTE
    {0x0000FB, 0x96}, // LATIN SMALL LETTER U WITH CIRCUMFLEX
    {0x0000FC, 0x81}, // LATIN SMALL LETTER U WITH DIAERESIS (ue)
    {0x0000FD,  'y'}, // LATIN SMALL LETTER Y WITH ACUTE (Muss natuerlich sein,
                      // um "týndur" einigermassen darzustellen)
    {0x0000FF, 0x98}, // LATIN SMALL LETTER Y WITH DIAERESIS
    {0x000192, 0x9F}, // LATIN SMALL LETTER F WITH HOOK
    {0x000393, 0xE2}, // GREEK CAPITAL LETTER GAMMA
    {0x000398, 0xE9}, // GREEK CAPITAL LETTER THETA
    {0x0003A3, 0xE4}, // GREEK CAPITAL LETTER SIGMA
    {0x0003A6, 0xE8}, // GREEK CAPITAL LETTER PHI
    {0x0003A9, 0xEA}, // GREEK CAPITAL LETTER OMEGA
    {0x0003B1, 0xE0}, // GREEK SMALL LETTER ALPHA
    {0x0003B2, 0xE1}, // GREEK SMALL LETTER BETA
    {0x0003B4, 0xEB}, // GREEK SMALL LETTER DELTA
    {0x0003B5, 0xEE}, // GREEK SMALL LETTER EPSILON
    {0x0003C0, 0xE3}, // GREEK SMALL LETTER PI
    {0x0003C3, 0xE5}, // GREEK SMALL LETTER SIGMA
    {0x0003C4, 0xE7}, // GREEK SMALL LETTER TAU
    {0x0003C6, 0xED}, // GREEK SMALL LETTER PHI
    {0x002018, '\''}, // LEFT SINGLE QUOTATION MARK
    {0x002019, '\''}, // RIGHT SINGLE QUOTATION MARK
    {0x00201A, '\''}, // SINGLE LOW-9 QUOTATION MARK
    {0x00201B, '\''}, // SINGLE HIGH-REVERSED-9 QUOTATION MARK
    {0x00201C,  '"'}, // LEFT DOUBLE QUOTATION MARK
    {0x00201D,  '"'}, // RIGHT DOUBLE QUOTATION MARK
    {0x00201E,  '"'}, // DOUBLE LOW-9 QUOTATION MARK
    {0x00201F,  '"'}, // DOUBLE HIGH-REVERSED-9 QUOTATION MARK
    {0x002022, 0x07}, // BULLET
    {0x00203C, 0x13}, // DOUBLE EXCLAMATION MARK (!!)
    {0x00207F, 0xFC}, // SUPERSCRIPT LATIN SMALL LETTER N (^n)
    {0x0020A7, 0x9E}, // PESETA SIGN (Pts)
    {0x0020AC, 0xEE}, // EURO SIGN (Das ist nicht wirklich ein EUR-Zeichen,
                      // aber es sieht ein bisschen so aus)
    {0x002190, 0x1B}, // LEFTWARDS ARROW (<-)
    {0x002191, 0x18}, // UPWARDS ARROW (^)
    {0x002192, 0x1A}, // RIGHTWARDS ARROW (->)
    {0x002193, 0x19}, // DOWNWARDS ARROW (v)
    {0x002194, 0x1D}, // LEFT RIGHT ARROW (<->)
    {0x002195, 0x12}, // UP DOWN ARROW
    {0x0021A8, 0x17}, // UP DOWN ARROW WITH BASE
    {0x002219, 0xF9}, // BULLET OPERATOR
    {0x00221A, 0xFB}, // SQUARE ROOT
    {0x00221E, 0xEC}, // INFINITY
    {0x00221F, 0x1C}, // RIGHT ANGLE
    {0x002229, 0xEF}, // INTERSECTION
    {0x002248, 0xF7}, // ALMOST EQUAL TO
    {0x002261, 0xF0}, // IDENTICAL TO
    {0x002264, 0xF3}, // LESS-THAN OR EQUAL TO
    {0x002265, 0xF2}, // GREATER-THAN OR EQUAL TO
    {0x002302, 0x7F}, // HOUSE
    {0x002310, 0xA9}, // REVERSED NOT SIGN
    {0x002320, 0xF4}, // TOP HALF INTEGRAL
    {0x002321, 0xF5}, // BOTTOM HALF INTEGRAL
    {0x002500, 0xC4}, // BOX DRAWINGS LIGHT HORIZONTAL
    {0x002502, 0xB3}, // BOX DRAWINGS LIGHT VERTICAL
    {0x00250C, 0xDA}, // BOX DRAWINGS LIGHT DOWN AND RIGHT
    {0x002510, 0xBF}, // BOX DRAWINGS LIGHT DOWN AND LEFT
    {0x002514, 0xC0}, // BOX DRAWINGS LIGHT UP AND RIGHT
    {0x002518, 0xD9}, // BOX DRAWINGS LIGHT UP AND LEFT
    {0x00251C, 0xC3}, // BOX DRAWINGS LIGHT VERTICAL AND RIGHT
    {0x002524, 0xB4}, // BOX DRAWINGS LIGHT VERTICAL AND LEFT
    {0x00252C, 0xC2}, // BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
    {0x002534, 0xC1}, // BOX DRAWINGS LIGHT UP AND HORIZONTAL
    {0x00253C, 0xC5}, // BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
    {0x002550, 0xCD}, // BOX DRAWINGS DOUBLE HORIZONTAL
    {0x002551, 0xBA}, // BOX DRAWINGS DOUBLE VERTICAL
    {0x002552, 0xD5}, // BOX DRAWINGS DOWN SINGLE AND RIGHT DOUBLE
    {0x002553, 0xD6}, // BOX DRAWINGS DOWN DOUBLE AND RIGHT SINGLE
    {0x002554, 0xC9}, // BOX DRAWINGS DOUBLE DOWN AND RIGHT
    {0x002555, 0xB8}, // BOX DRAWINGS DOWN SINGLE AND LEFT DOUBLE
    {0x002556, 0xB7}, // BOX DRAWINGS DOWN DOUBLE AND LEFT SINGLE
    {0x002557, 0xBB}, // BOX DRAWINGS DOUBLE DOWN AND LEFT
    {0x002558, 0xD4}, // BOX DRAWINGS UP SINGLE AND RIGHT DOUBLE
    {0x002559, 0xD3}, // BOX DRAWINGS UP DOUBLE AND RIGHT SINGLE
    {0x00255A, 0xC8}, // BOX DRAWINGS DOUBLE UP AND RIGHT
    {0x00255B, 0xBE}, // BOX DRAWINGS UP SINGLE AND LEFT DOUBLE
    {0x00255C, 0xBD}, // BOX DRAWINGS UP DOUBLE AND LEFT SINGLE
    {0x00255D, 0xBC}, // BOX DRAWINGS DOUBLE UP AND LEFT
    {0x00255E, 0xC6}, // BOX DRAWINGS VERTICAL SINGLE AND RIGHT DOUBLE
    {0x00255F, 0xC7}, // BOX DRAWINGS VERTICAL DOUBLE AND RIGHT SINGLE
    {0x002560, 0xCC}, // BOX DRAWINGS DOUBLE VERTICAL AND RIGHT
    {0x002561, 0xB5}, // BOX DRAWINGS VERTICAL SINGLE AND LEFT DOUBLE
    {0x002562, 0xB6}, // BOX DRAWINGS VERTICAL DOUBLE AND LEFT SINGLE
    {0x002563, 0xB9}, // BOX DRAWINGS DOUBLE VERTICAL AND LEFT
    {0x002564, 0xD1}, // BOX DRAWINGS DOWN SINGLE AND HORIZONTAL DOUBLE
    {0x002565, 0xD2}, // BOX DRAWINGS DOWN DOUBLE AND HORIZONTAL SINGLE
    {0x002566, 0xCB}, // BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL
    {0x002567, 0xCF}, // BOX DRAWINGS UP SINGLE AND HORIZONTAL DOUBLE
    {0x002568, 0xD0}, // BOX DRAWINGS UP DOUBLE AND HORIZONTAL SINGLE
    {0x002569, 0xCA}, // BOX DRAWINGS DOUBLE UP AND HORIZONTAL
    {0x00256A, 0xD8}, // BOX DRAWINGS VERTICAL SINGLE AND HORIZONTAL DOUBLE
    {0x00256B, 0xD7}, // BOX DRAWINGS VERTICAL DOUBLE AND HORIZONTAL SINGLE
    {0x00256C, 0xCE}, // BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
    {0x002580, 0xDF}, // UPPER HALF BLOCK
    {0x002584, 0xDC}, // LOWER HALF BLOCK
    {0x002588, 0xDB}, // FULL BLOCK
    {0x00258C, 0xDD}, // LEFT HALF BLOCK
    {0x002590, 0xDE}, // RIGHT HALF BLOCK
    {0x002591, 0xB0}, // LIGHT SHADE
    {0x002592, 0xB1}, // MEDIUM SHADE
    {0x002593, 0xB2}, // DARK SHADE
    {0x0025A0, 0xFE}, // BLACK SQUARE
    {0x0025AC, 0x16}, // BLACK RECTANGLE
    {0x0025B2, 0x1E}, // BLACK UP-POINTING TRIANGLE
    {0x0025BA, 0x10}, // BLACK RIGHT-POINTING POINTER
    {0x0025BC, 0x1F}, // BLACK DOWN-POINTING TRIANGLE
    {0x0025C4, 0x11}, // BLACK LEFT-POINTING POINTER
    {0x0025CB, 0x09}, // WHITE CIRCLE
    {0x0025D8, 0x08}, // INVERSE BULLET
    {0x0025D9, 0x0A}, // INVERSE WHITE CIRCLE
    {0x00263A, 0x01}, // WHITE SMILING FACE
    {0x00263B, 0x02}, // BLACK SMILING FACE
    {0x00263C, 0x0F}, // WHITE SUN WITH RAYS
    {0x002640, 0x0C}, // FEMALE SIGN
    {0x002642, 0x0B}, // MALE SIGN
    {0x002660, 0x06}, // BLACK SPADE SUIT
    {0x002663, 0x05}, // BLACK CLUB SUIT
    {0x002665, 0x03}, // BLACK HEART SUIT
    {0x002666, 0x04}, // BLACK DIAMOND SUIT
    {0x00266A, 0x0D}, // EIGHTH NOTE
    {0x00266B, 0x0E}, // BEAMED EIGHTH NOTES
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
 * @return true wenn das Zeichen umgewandelt wurde, false sonst
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
        return false;
    } else {
        *dest = to_cp437(wc);
        vterm->utf8_buffer_offset = 0;

        // Bei einem Zeichen, das nicht kopiert werden kann, wird ein ? als
        // Platzhalter geschrieben
        if (!*dest) {
            *dest = '?';
        }
        return true;
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

