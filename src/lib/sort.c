/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Mathias Gottschlag.
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

static void swap(void* a, void* b, size_t size)
{
    if (a == b) {
        return;
    }

    char temp[size];
    memcpy(temp, a, size);
    memcpy(a, b, size);
    memcpy(b, temp, size);
}

void qsort(void* base, size_t num, size_t size,
    int (*comparator)(const void*, const void*))
{
    if (num < 2) {
        return;
    }

    size_t left = 1, right = num - 1;

    /*
     * Als Pivot-Element p ist das Element mit Index 0 gewählt. Das Array wird
     * grob vorsortiert indem alles was kleiner p ist nach links und alles was
     * größer oder gleich p ist nach rechts gebracht wird.
     */
    while (left <= right) {
        while ((left <= right) && (comparator(base, base + left * size) > 0)) {
            ++left;
        }

        while ((left < right) && (comparator(base, base + right * size) <= 0)) {
            --right;
        }

        if (left >= right) {
            break;
        }

        swap(base + left * size, base + right * size, size);
        ++left;
        --right;
    }

    /*
     * Hier angekommen zeigt left auf das erste Element, welches nicht kleiner
     * ist als das Pivot-Element. left soll aber auf das letzte Element zeigen
     * das kleiner ist als das Pivot-Element. right könnte auf dieses zeigen,
     * soll aber auf das erste nicht kleiner Element zeigen.
     */
    right = left--;

    /* Pivot-Element einsortieren */
    swap(base, base + left * size, size);

    /* Linke Seite sortieren. */
    qsort(base, left, size, comparator);

    /* Rechte Seite sortieren. */
    qsort(base + right * size, num - right, size, comparator);
}
