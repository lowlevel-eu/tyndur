/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
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

#include "keymap.h"

keymap_entry_t keymap[128] = {
    { 0,        0,      0,      0 },        // 0
    { '\033', '\033', '\033', '\033' },
    { '1',    '!',    '|',      0 },
    { '2',    '@',      0,      0 },
    { '3',    '#',      0,      0 },
    { '4',    '$',      0,      0 },
    { '5',    '%',      0,      0 },
    { '6',    '^',      0,      0 },
    { '7',    '&',      0,      0 },
    { '8',    '*',      0,      0 },
    { '9',    '(',      0,      0 },        // 10
    { '0',    ')',      0,      0 },
    { '-',    '_',      0,      0 },
    { '=',    '+',      0,      0 },
    { '\b',     0,      0,      0 },
    { '\t',     0,      0,      0 },
    { 'q',    'Q',    '@',     17 },
    { 'w',    'W',      0,     23 },
    { 'e',    'E',      0,      5 },
    { 'r',    'R',      0,     18 },
    { 't',    'T',      0,     20 },        // 20
    { 'y',    'Y',      0,     26 },
    { 'u',    'U',   L'ü',     21 },
    { 'i',    'I',      0,      9 },
    { 'o',    'O',   L'ö',     15 },
    { 'p',    'P',      0,     16 },
    { '[',    '{',      0,      0 },
    { ']',    '}',      0,      0 },
    { '\n',     0,      0,      0 },
    { 0,        0,      0,      0 },
    { 'a',    'A',   L'ä',      1 },        // 30
    { 's',    'S',   L'ß',     19 },
    { 'd',    'D',      0,      4 },
    { 'f',    'F',      0,      6 },
    { 'g',    'G',      0,      7 },
    { 'h',    'H',      0,      8 },
    { 'j',    'J',      0,     10 },
    { 'k',    'K',      0,     11 },
    { 'l',    'L',      0,     12 },
    { ';',    ':',      0,      0 },
    { '\'',   '"',      0,      0 },        // 40
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { '\\',   '|',      0,      0 },
    { 'z',    'Z',      0,     25 },
    { 'x',    'X',      0,     24 },
    { 'c',    'C',      0,      3 },
    { 'v',    'V',      0,     22 },
    { 'b',    'B',      0,      2 },
    { 'n',    'N',      0,     14 },
    { 'm',    'M',      0,     13 },        // 50
    { ',',    '<',      0,      0 },
    { '.',    '>',      0,      0 },
    { '/',    '?',      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { ' ',    ' ',    ' ',      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },        // 60
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },        // 70
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },        // 80
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { '<',    '>',    '|',      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },        // 90
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },        // 100
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },        // 110
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },        // 120
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
    { 0,        0,      0,      0 },
};
