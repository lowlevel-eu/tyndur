/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
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

/**
 * Fuehrt eine Suche in einem sortierten Array von Objekten durch
 *
 * @param key Schluesselobjekt, das zum Vergleich mit dem gesuchten Objekt
 * verwendet wird
 * @param base Zeiger auf den Anfang des Arrays, das durchsucht wird
 * @param nel Anzahl der Elemente im Array
 * @param width Die Groesse eines Elements in bytes
 * @param compar Vergleichsfunktion, die angewandt wird, um die Suche
 * durchzufuehren. Jeder Kandidat wird durch diese Funktion mit dem
 * Schluesselobjekt verglichen. Wenn die Objekte gleich sind, gibt die
 * Vergleichsfunktion 0 zurueck und die Suche ist erfolgreich beendet.
 * Ansonsten gibt die Vergleichsfunktion < 0 zurueck, wenn der Schluessel
 * kleiner als der Kandidat ist; sie gibt > 0 zurueck, wenn die Schluessel
 * groesser ist
 *
 * @return Die Funktion gibt das gesuchte Objekt zurueck; wenn kein Objekt
 * gefunden wurde, gibt sie NULL zurueck.
 */
void *bsearch(const void *key, const void *base, size_t nel,
    size_t width, int (*compar)(const void *, const void *))
{
    const char* obj;

    if (nel == 0) {
        return NULL;
    }

    obj = base + (width * (nel >> 1));
    int cmp = compar(key, obj);

    if (cmp == 0) {
        return (void*)obj;
    } else if (cmp < 0) {
        if (nel >> 1 == 0) {
            return NULL;
        }
        return bsearch(key, base, nel - (nel >> 1), width, compar);
    } else {
        return bsearch(key, obj + width, nel - (nel >> 1) - 1, width, compar);
    }
}

