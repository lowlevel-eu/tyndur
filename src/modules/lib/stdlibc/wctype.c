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

#include <wctype.h>
#include <string.h>

static struct { const char* name; wctype_t func; } types[] = {
    { "alnum", iswalnum },
    { "alpha", iswalpha },
    { "blank", iswblank },
    { "cntrl", iswcntrl },
    { "digit", iswdigit },
    { "graph", iswgraph },
    { "lower", iswlower },
    { "print", iswprint },
    { "punct", iswpunct },
    { "space", iswspace },
    { "upper", iswupper },
    { "xdigit", iswxdigit }
};


/**
 * Erstellt ein wctype-Objekt
 *
 * @param type Typ des zu erstellenden Objekts
 *
 * @return Das Objekt, ansonsten (wctype_t)0
 **/
wctype_t wctype(const char* name)
{
    int i;

    for (i = 0; i < sizeof(types) / sizeof(types[0]); i++)
    {
        if (!strcmp(name, types[i].name)) {
            return types[i].func;
        }
    }

    return (wctype_t)0;
}


/**
 * Prüft, ob der als Parameter übergebene Typ auch vom Typ wctype_t ist
 *
 * @param wc Übergebener Typ
 * @param type Typklasse
 *
 * @return 0, wenn die Klasse nicht vorhanden ist, ansonsten den
 * Funktionspointer
 **/
int iswctype(wint_t wc, wctype_t type)
{
    return type(wc);
}


/**
 * Konvertiert ein Ein-Byte-Character in ein Wide-Character
 *
 * @param c Der Character
 *
 * @return Das konvertierte Zeichen, ansonsten WEOF
 **/
wint_t btowc(int c)
{
    return (c < 0 || c > 127) ? WEOF : (wint_t)c;
}


/**
 * Testet ob es sich bei einem breiten Zeichen um ein grosses alphabetisches
 * Zeichen handelt.
 * @see iswlower
 * @see iswalpha
 * @see towupper
 * @see towlower
 * @see isupper
 */
int iswupper(wint_t wc)
{
    return (wc >= L'A' && wc <= L'Z');
}

/**
 * Testet ob es sich bei einem breiten Zeichen um ein kleines alphabetisches
 * Zeichen handelt.
 * @see iswupper
 * @see iswalpha
 * @see towlower
 * @see towupper
 * @see islower
 */
int iswlower(wint_t wc)
{
    return (wc >= L'a' && wc <= L'z');
}

/**
 * Testet ob es sich bei einem breiten Zeichen um ein alphabetisches Zeichen
 * handelt.
 * @see iswalnum
 * @see isalpha
 */
int iswalpha(wint_t wc)
{
    return ((wc >= L'a' && wc <= L'z') || (wc >= L'A' && wc <= L'Z'));
}

/**
 * Testet ob es sich bei einem breiten Zeichen um eine Ziffer handelt.
 * @see iswalnum
 * @see isdigit
 */
int iswdigit(wint_t wc)
{
    return (wc >= L'0' && wc <= L'9');
}

/**
 * Testet ob es sich bei einem breiten Zeichen um ein alphabetisches Zeichen
 * oder eine Ziffer handelt.
 * @see iswalpha
 * @see iswdigit
 * @see isalnum
 */
int iswalnum(wint_t wc)
{
    return (iswdigit(wc) || iswalpha(wc));
}

/**
 * Testen ob es sich bei einem Breiten Zeichen um ein Leerzeichen oder ein
 * Zeilenende handelt.
 * @see iswblank
 * @see isblank
 */
int iswspace(wint_t wc)
{
    return (wc == L' ' || wc == L'\n' || wc == L'\r' || wc == L'\t');
}

/**
 * Testen ob es sich bei einem breiten Zeichen um ein Leerzeichen
 * handelt(einfaches Leerzeichen oder Tab).
 * @see iswspace
 * @see isblank
 */
int iswblank(wint_t wc)
{
    return (wc == L' ' || wc == L'\t');
}

/**
 * TODO: Kann man das irgendwie sinnvoll erklaeren, was da alles
 *       dazugehoert? ;-) Mit Unicode sind das naemlich nicht nur
 *       Punktuationszeichen sondern auch andere Sonderzeichen.
 * @see ispunct
 */
int iswpunct(wint_t wc)
{
    return iswprint(wc) && !iswspace(wc) && !iswalnum(wc);
}

/**
 * Testen ob es sich bei einem breiten Zeichen um ein druckbares Zeichen
 * handelt. Das sind alle Zeichen, die keine Kontrollzeichen(iswcntrl) sind.
 * @see iswcntrl
 * @see isprint
 */
int iswprint(wint_t wc)
{
    return (wc >= (wint_t) 0x20);
}

/**
 * Testen ob es sich bei einem breiten Zeichen um ein Kontrollzeichen(z.B \n, \t
 * oder \b) handelt. Diese Zeichen wuerden bei einem iswprint-Aufruf 0
 * zurueckgeben.
 * @see iswprint
 * @see iscntrl
 */
int iswcntrl(wint_t wc)
{
    return (wc < (wint_t) 0x20);
}

/**
 * Testen, ob das Zeichen ein druckbares Zeichen ist, ausgenommen davon ist
 * ein Leerzeichen.
 **/
int iswgraph(wint_t wc)
{
    return iswprint(wc) && !iswspace(wc);
}

/**
 * Testen, ob das Zeichen eine druckbare Hexadezimale Ziffer ist
 **/
int iswxdigit(wint_t wc)
{
    return ((wc >= L'A' && wc <= L'F') || (wc >= L'a' && wc <= L'f')
        || (wc >= L'0' && wc <= '9'));
}

/**
 * Breites Zeichen in einen Grossbuchstaben umwandeln, falls es sich um einen
 * Kleinbuchstaben handelt. Sonst wird das Zeichen unveraendert zurueckgegeben.
 * @see iswupper
 * @see towlower
 * @see tolower
 */
wint_t towupper(wint_t wc)
{
    return iswlower(wc) ? wc - (L'a' - L'A') : wc;
}

/**
 * Breites Zeichen in einen Kleinbuchstaben umwandeln, falls es sich um einen
 * Grossbuchstaben umwandeln. Sonst wird das Zeichen unveraendert
 * zurueckgegeben.
 * @see iswlower
 * @see towupper
 * @see tolower
 */
wint_t towlower(wint_t wc)
{
    return iswupper(wc) ? wc + (L'a' - L'A') : wc;
}

