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

#ifndef _WCHAR_H_
#define _WCHAR_H_

#include <stddef.h>
#include <stdio.h>

#define WEOF ((wint_t) -1)

/**
 * Sollte theoretisch alle Werte von wchar_t und zusaetzlich WEOF annehmen
 * koennen.
 */
typedef wchar_t wint_t;

/**
 * Repraesentiert den internen Shift-Status einer Funktion, brauchen wir mit
 * UTF-8 nicht.
 */
typedef int mbstate_t;



/* WSTRINGS */

/**
 * Reentrante Variante von mbtowc, bei uns mit UTF-8 aber identisch.
 * @see mbtowc
 */
size_t mbrtowc(wchar_t* wc, const char* s, size_t len, mbstate_t* ps);

/**
 * Reentrante Variante von wctomb, bei uns mit UTF-8 aber identisch.
 * @see wctomb
 */
size_t wcrtomb(char* buf, wchar_t wc, mbstate_t* ps);

/**
 * Reentrante Variante von wcstombs. Der einzige wesentliche Unterschied fuer
 * uns mit UTF8 ist, dass *wcs so aktualisiert wird, dass es bei einem Abbruch,
 * sei es weil buf zu klein ist oder weil ein ungueltiges Zeichen angetroffen
 * wurde, auf das betreffende Zeichen zeigt. Wird der String erfolgreich
 * verarbeitet, wird *wcs auf NULL gesetzt.
 * @see wcstombs
 */
size_t wcsrtombs(char* buf, const wchar_t** wcs, size_t len, mbstate_t* ps);

/**
 * Reentrante Variante von mbstowcs. Der einzige wesentliche Unterschied fuer
 * uns mit UTF8 ist, dass *str so aktualisiert wird, dass es bei einem Abbruch,
 * sei es weil buf zu klein ist oder weil ein ungueltiges Zeichen angetroffen
 * wurde, auf das betreffende Zeichen zeigt. Wird der String erfolgreich
 * verarbeitet, wird *str auf NULL gesetzt.
 * @see mbstowcs
 */
size_t mbsrtowcs(wchar_t* buf, const char** str, size_t len, mbstate_t* ps);


/**
 * Anzahl der Spalten, die ein Zeichen in Anspruch nimmt, errechnen. Fuer c = 0
 * wird 0 zuruekgegeben. Falls es sich nicht um ein druckbares Zeichen handelt,
 * wird -1 zurueckgegeben.
 *
 * @param wc Das breite Zeichen
 *
 * @return Anzahl Spalten, oder -1 wenn das Zeichen nicht druckbar ist.
 */
int wcwidth(wchar_t wc);

/**
 * Anzahl der Spalten, die ein String aus breiten Zeichen in Anspruch nimmt,
 * errechnen. Wird ein nicht druckbares Zeichen erkannt, wird abgebrochen.
 *
 * @see wcwidth
 * @param wcs Zeiger auf das erste Zeichen
 * @param len Anzahl der Zeichen
 *
 * @return Anzahl der Zeichen oder -1 im Fehlerfall.
 */
int wcswidth(const wchar_t* wcs, size_t len);


/**
 * Einen String aus breiten Zeichen kopieren, das ist das Pendant zu stpcpy. Die
 * beiden Strings duerfen sich nicht ueberlappen. Das abschliessende L'0' wird
 * mitkopiert.
 *
 * @see stpcpy
 * @see wcscpy
 * @see wcpncpy
 * @param dst Zeiger auf die Speicherstelle in die der Kopiertestring abgelegt
 *            werden soll. Dabei muss vom Aufrufer sichergestellt werden, dass
 *            dort mindestens speicher fuer wcslen(src) + 1 breite Zeichen ist.
 * @param src
 *
 * @return Zeiger auf das Ende (L'0') des Zielstrings.
 */
wchar_t* wcpcpy(wchar_t* dst, const wchar_t* src);

/**
 * String aus breiten Zeichen kopieren. Dabei werden hoechstens len Zeichen
 * kopiert. Sind in src weniger als len Zeichen wird dst mit L'\0' aufgefuellt.
 * Ist src laenger als oder gleich lang wie len wird der String in dst nich
 * nullterminiert, der Rueckgabewert zeigt also nich auf ein L'\0'. Die beiden
 * Speicherbereiche duerfen sich nicht ueberlappen. Diese Funktion ist das
 * Pendant zu strncpy.
 *
 * @see strncpy
 * @see wcpncpy
 * @param dst Zeiger auf den Speicherbereich der len breite Zeichen aufnehmen
 *            kann. Es werden genau len Zeichen hineingeschrieben.
 * @param src Quellstring
 * @param len Anzahl der Zeichen, die in dst geschrieben werden sollen.
 *
 * @return Zeiger auf das letzte geschriebene Byte, also immer dst + len - 1
 */
wchar_t* wcpncpy(wchar_t* dst, const wchar_t* src, size_t len);

/**
 * Zwei Strings aus breiten Zeichen ohne Unterscheidung von
 * Gross-/Kleinschreibung vergleichen inklusive abschliessendes L'\0'. Diese
 * Funktion ist das Pendant zu strcasecmp.
 *
 * @see strcasecmp
 * @see wcscmp
 * @see towlower
 * @param wcs1 Erster String
 * @param wcs2 Zweiter String
 *
 * @return 0 wenn die Strings gleich sind, oder <0 wenn das erste
 *         ungleiche Zeichen in wcs1 kleiner ist als das in wcs2, ist es
 *         groesser >0.
 */
int wcscasecmp(const wchar_t* wcs1, const wchar_t* wcs2);

/**
 * Zwei Strings aus breiten Zeichen aneinanderhaengen. Dabei wird der Inhalt von
 * src inklusive dem abschliessenden L'\0' ans Ende von dst kopiert. Der
 * Aufrufer muss sicherstellen, dass nach dst genug Speicher frei ist. Diese
 * Funktion ist das Pendant zu strcat.
 *
 * @see strcat
 * @see wcsncat
 * @param dst Zeiger auf den String an dessen Ende src angehaengt werden soll.
 * @param src Zeiger auf den String der an dst angehaengt werden soll.
 *
 * @return dst
 */
wchar_t* wcscat(wchar_t* dst, const wchar_t* src);

/**
 * Erstes Vorkommen eines breiten Zeichens aus einem String aus breiten Zeichen
 * heraussuchen. Diese Funktion ist das Pendant zu strchr.
 *
 * @see strchr
 * @param wcs Zeiger auf den String in dem gesucht werden soll.
 * @param wc  Zeichen das gesucht werden soll.
 *
 * @return Zeiger auf das erste gefundene Zeichen oder NULL wenn keines gefunden
 *         wurde.
 */
wchar_t* wcschr(const wchar_t* wcs, wchar_t wc);

/**
 * Zwei Strings aus breiten Zeichen inklusive dem abschliessenden L'\0'
 * vergleichen. Diese Funktion ist das Pendant zu strcmp.
 *
 * @param wcs1 Erster String
 * @param wcs2 Zweiter String
 *
 * @return 0 wenn die Strings gleich sind, oder <0 wenn das erste
 *         ungleiche Zeichen in wcs1 kleiner ist als das in wcs2, ist es
 *         groesser >0.
 */
int wcscmp(const wchar_t* wcs1, const wchar_t* wcs2);

/**
 * TODO
 */
int wcscoll(const wchar_t* wcs1, const wchar_t* wcs2);

/**
 * String aus Breiten Zeichen inklusive abschliessendem L'\0' kopieren. Der
 * Aufrufer hat sicherzustellen, dass dst genug Platz bietet. Die beiden Strings
 * duerfen sich nicht uberlappen. Diese Funktion ist das Pendant zu strcpy.
 *
 * @see strcpy
 * @see wcsncpy
 * @see wcpcpy
 * @param dst Zeiger auf den Speicherbereich in dem die Kopie abgelegt werden
 *            soll.
 * @param src Zeiger auf den String der kopiert werden soll.
 *
 * @return dst
 */
wchar_t* wcscpy(wchar_t* dst, const wchar_t* src);

/**
 * String aus breiten Zeichen nach einem Vorkommen eines Zeichens aus set
 * durchsuchen. Diese Funktion ist das Pendant zu strcspn.
 *
 * @see strcspn
 * @see wcsspn
 * @param wcs Zeiger auf den String, dar durchsucht werden soll
 * @param set Zeiger auf den String mit den zu suchenden Zeichen.
 *
 * @return Offset vom Anfang des Strings oder wcslen(wcs) wenn kein Vorkommen
 *         gefunden wurde.
 */
size_t wcscspn(const wchar_t* wcs, const wchar_t* set);

/**
 * String aus breiten Zeichen in einen neu allozierten Speicherbereich kopieren.
 * Diese Funktion ist das Pendant zu strdup.
 *
 * @see strdup
 * @param wcs Zeiger auf den zu kopierenden String
 *
 * @return Zeiger auf die Kopie. Muss vom Aufrufer freigegeben werden. Im
 *         Fehlerfall NULL.
 */
wchar_t* wcsdup(const wchar_t* wcs);

/**
 * Kopiert src an das Ende des Strings dst. Dabei wird der String dst auf
 * maximal len - 1 Zeichen und L'\0' verlaengert, ausser wenn len == 0.
 *
 * @see wcsncat
 * @param dst Zeiger auf String an den src angehaengt werden soll
 * @param src Zeiger auf String der an dst angehaengt werden soll
 * @param len Anzahl Zeichen auf die dst Maximal verlaengert werden darf
 *            inklusiv L'\0.'
 *
 * @return wcslen(urspruengliches dst) + wcslen(src); Wenn der Rueckgabewert >=
 *         len ist, wurde src nich vollstaendig kopiert.
 */
size_t wcslcat(wchar_t* dst, const wchar_t* src, size_t len);

/**
 * Kopiert einen String aus breiten Zeichen. Dabei werden maximal len - 1
 * Zeichen kopiert, und dst wird immer mit L'\0' terminiert, wenn len != 0 ist.
 *
 * @see wcscpy
 * @param dst Zeiger auf den Speicherbereich in dem die Kopie abgelegt werden
 *            soll.
 * @param src Zeiger auf den Quellstring
 * @param len Anzahl der Zeichen inklusiv abschliessendes L'\0', die maximal in
 *            dst geschrieben werden sollen.
 *
 * @return wcslen(src); Wenn der Rueckgabewert <=  len ist, wurde src nicht
 *         vollstaendig kopiert.
 */
size_t wcslcpy(wchar_t* dst, const wchar_t* src, size_t len);

/**
 * Laenge eines Strings aus breiten Zeichen bestimmen. Dabei werden die Zeichen
 * gezaehlt, bis ein abschliessendes L'\0' gefunden wird. Das L'\0' wird nicht
 * mitgezaehlt. Diese Funktion ist das Pendant zu strlen.
 *
 * @see strlen
 * @param wcs Zeiger auf den String
 *
 * @return Laenge des Strings in Zeichen
 */
size_t wcslen(const wchar_t* wcs);

/**
 * Zwei Strings aus breiten Zeichen ohne Unterscheidung von
 * Gross-/Kleinschreibung vergleichen. Dabei werden maximal die ersten len
 * Zeichen von Beiden Strings verglichen. Diese Funktion ist das Pendant zu
 * strncasecmp.
 *
 * @see strncasecmp
 * @see wcscasecmp
 * @param wcs1 Erster String
 * @param wcs2 Zweiter String
 * @param len  Anzahl der Zeichen, die maximal verglichen werden sollen, wenn
 *             vorher kein L'\0' gefunden wird.
 *
 * @return 0 wenn die beiden Strings gleich sind, < 0 wenn das Zeichen in wcs2
 *         groesser ist als das in wcs1 und > 0 wenn das Zeichen in wcs1
 *         groesser ist als das in wcs2.
 */
int wcsncasecmp(const wchar_t* wcs1, const wchar_t* wcs2, size_t len);

/**
 * Zwei Strings aus breiten Zeichen aneinanderhaengen. Dabei werden maximal len
 * Zeichen aus src kopiert. Der Aufrufer muss sicherstellen, dass nach dem
 * String, auf den dst zeigt, noch mindestens len + 1 Zeichen platz haben. dst
 * wird in jedem Fall mit L'\0' terminiert. Diese Funktion ist das Pendant zu
 * strncat.
 *
 * @see strncat
 * @see wcscat
 * @param dst String an den src angehaengt werden soll
 * @param src String der an dst angehaengt werden soll
 * @param len Anzahl der Zeichen, die maximal kopiert werden aus src
 *
 * @return dst
 */
wchar_t* wcsncat(wchar_t* dst, const wchar_t* src, size_t len);

/**
 * Zwei Strings aus breiten Zeichen vergleichen. Dabei werden maximal len
 * Zeichen verglichen. Diese Funktion ist das Pendant zu strncmp.
 *
 * @see strncmp
 * @see wcscmp
 * @param wcs1 Erster String
 * @param wcs2 Zweiter String
 * @param len  Anzahl der Zeichen, die maximal verglichen werden sollen, wenn
 *             vorher kein abschliessendes L'\0' angetroffen wurde.
 *
 * @return 0 wenn die Strings gleich sind, oder <0 wenn das erste
 *         ungleiche Zeichen in wcs1 kleiner ist als das in wcs2, ist es
 *         groesser >0.
 */
int wcsncmp(const wchar_t* wcs1, const wchar_t* wcs2, size_t len);

/**
 * String aus breiten Zeichen kopieren. Dabei werden maximal len Zeichen
 * kopiert. Die restlichen Zeichen im Puffer werden mit L'\0' gefuellt. Ist src
 * gleich lang wie oder laenger als len, wird dst nicht mit L'\0' terminiert.
 * Die beiden Strings duerfen sich nicht ueberlappen. Diese Funktion ist das
 * Pendant zu strncpy.
 *
 * @see strncpy
 * @see wcscpy
 * @param src String der kopiert werden soll
 * @param dst Zeiger auf den Speicherbereich in den src kopiert werden soll
 * @param len Anzahl der Zeichen, die maximal kopiert werden sollen
 *
 * @return dst
 */
wchar_t* wcsncpy(wchar_t* dst, const wchar_t* src, size_t len);

/**
 * Laenge eines Strings aus breiten Zeichen errechnen. Dabei werden maximal max
 * Zeichen gezaehlt. Diese Funktion ist das Pendant zu strnlen.
 *
 * @see strnlen
 * @see wcslen
 * @param wcs String
 * @param max Anzahl der Zeichen, die maximal gezaehlt werden sollen
 *
 * @return Anzahl der Zeichen ohne abschliessendes L'\0'
 */
size_t wcsnlen(const wchar_t* wcs, size_t max);

/**
 * Erstes vorkommen eines breiten Zeichens aus set im String aus breiten Zeichen
 * wcs suchen. Diese Funktion ist das Pendant zu strpbrk.
 *
 * @see strpbrk
 * @see wcschr
 * @param wcs String der durchsucht werden soll
 * @param set String aus Zeichen nach denen gesucht werden soll
 *
 * @return Zeiger auf die Position an der ein Zeichen gefunden wurde oder NULL
 *         falls keines gefunden wurde.
 */
wchar_t* wcspbrk(const wchar_t* wcs, const wchar_t* set);

/**
 * Letztes Vorkommen eines breiten Zeichens in einem String aus breiten Zeichen
 * suchen. Diese Funktion ist das Pendant zu strrchr.
 *
 * @see strrchr
 * @see wcschr
 * @param wcs String der durchsucht werden soll
 * @param wc  Zeichen das gesucht werden soll
 *
 * @return Zeiger auf das letzte gefundene Vorkommen, oder NULL falls das
 *         Zeichen nicht gefunden wurde.
 */
wchar_t* wcsrchr(const wchar_t* wcs, wchar_t wc);

/**
 * Durchsucht einen String aus breiten Zeichen nach dem ersten Zeichen das nicht
 * in set vorkommt. Diese Funktion ist das Pendant zu strspn.
 *
 * @see strspn
 * @see wcschr
 * @param wcs String der durchsucht werden soll
 * @param set Zeichen die zugelassen sind
 *
 * @return Offset des ersten Zeichens das nicht in set vorkommt, oder
 * wcslen(wcs) falls alle Zeichen in set enthalten sind.
 */
size_t wcsspn(const wchar_t* wcs, const wchar_t* set);

/**
 * Erstes Vorkommen des Breiten Strings find in wcs suchen. Diese Funktion ist
 * das Pendant zu strstr.
 *
 * @see strstr
 * @see wcschr
 * @param wcs  String der durchsucht werden soll
 * @param find String der gesucht werden soll
 *
 * @return Zeiger auf das erste Vorkommen in wcs oder NULL falls keines gefunden
 *         wurde.
 */
wchar_t* wcsstr(const wchar_t* wcs, const wchar_t* find);

/**
 * String aus breiten Zeichen in Tokens aufspalten, die durch die in delim
 * angegebenen Zeichen getrennt werden. wcs wird dabei veraendert. Ist wcs !=
 * NULL beginnt die suche dort, sonst wird bei *last begonnen. *last wird
 * jeweils auf den Anfang des naechsten Token gesetzt, oder auf NULL, wenn das
 * Ende erreicht wurde.
 * Diese Funktion ist das Pendant zu strtok.
 *
 * @see strtok
 * @param wcs   Zeiger auf das Zeichen bei dem die Zerlegung in Tokens begonnen
 *              werden soll, oder NULL, wenn der Wert von *last genommen werden
 *              soll.
 * @param delim Zeichen die zwei Tokens voneinander trennen koennen
 * @param last  Zeiger auf die Speicherstelle an der die Funktion die Position
 *              des naechsten token speichern kann fuer den internen Gebrauch,
 *              um beim Naechsten Aufruf mit wcs == NULL das naechste Token
 *              zurueck geben zu koennen.
 *
 * @return Zeiger auf das aktuelle Token, oder NULL wenn keine Tokens mehr
 *         vorhanden sind.
 */
wchar_t* wcstok(wchar_t* wcs, const wchar_t* delim, wchar_t** last);

/**
 * Speicherbereich aus breiten Zeichen nach einem bestimmten Zeichen
 * durchsuchen. Diese Funktion ist das Pendant zu memchr.
 *
 * @see memchr
 * @see wcschr
 * @param wcs Zeiger auf den Speicherbereich
 * @param wc  Zu suchendes Zeichen
 * @param len Laenge des Speicherbereichs
 *
 * @return Zeiger auf das gefundene Zeichen oder NULL falls keines gefunden
 *         wurde.
 */
wchar_t* wmemchr(const wchar_t* wcs, wchar_t wc, size_t len);

/**
 * Zwei Speicherbereiche aus breiten Zeichen vergleichen. Diese Funktion ist das
 * Pendant zu memcmp.
 *
 * @see memcmp
 * @see wcscpm
 * @see wcsncmp
 * @param wcs1 Zeiger auf den ersten Speicherbereich
 * @param wcs2 Zeiger auf den zweiten Speicherbereich
 * @param len  Laenge der beiden Speicherbereiche
 *
 * @return 0 wenn die beiden Speicherbereiche gleich sind, < 0 wenn das erste
 *         unterschiedliche Zeichen in wcs1 kleiner ist als das
 *         Korrespondierende in wcs2 oder > 0 wenn es groesser ist.
 */
int wmemcmp(const wchar_t* wcs1, const wchar_t* wcs2, size_t len);

/**
 * Speicherbereich aus breiten Zeichen kopieren. Die beiden Speicherbereiche
 * duerfen sich nicht ueberlappen. Diese Funktion ist das Pendant zu memcpy.
 *
 * @see memcpy
 * @see wcsncpy
 * @see wcscpy
 * @param dst Speicherbereich in den kopiert werden soll
 * @param src Speicherbereich der kopiert werden soll
 * @param len Laenge der Speicherbereiche
 *
 * @return dst
 */
wchar_t* wmemcpy(wchar_t* dst, const wchar_t* src, size_t len);

/**
 * Speicherbereich aus breiten Zeichen kopieren. Die beiden Speicherbereiche
 * duerfen sich ueberlappen. Diese Funktion ist das Pendant zu memcpy.
 *
 * @see memmove
 * @see wmemcpy
 * @param dst Speicherbereich in den kopiert werden soll
 * @param src Speicherbereich der kopiert werden soll
 * @param len Laenge der Speicherbereiche
 *
 * @return dst
 */
wchar_t* wmemmove(wchar_t* dst, const wchar_t* src, size_t len);

/**
 * Speicherbereich aus breiten Zeichen mit einem bestimmten Zeichen ausfuellen.
 * Diese Funktion ist das pendant zu memset.
 *
 * @see memset
 * @param wcs Zeiger auf den Speicherbereich
 * @param wc  Zeichen mit dem der Speicherbereich gefuellt werden soll
 * @param len Laenge des Speicherbereichs
 *
 * @return wcs
 */
wchar_t* wmemset(wchar_t* wcs, wchar_t wc, size_t len);




/* WSTDIO */

/**
 * Breites Zeichen aus einer Datei lesen.
 *
 * @param stream Die geoeffnete Datei
 *
 * @return Das gelesene Zeichen oder WEOF im Fehlerfall.
 */
wint_t fgetwc(FILE* stream);

/**
 * Wie fgetwc, mit dem Unterschied, dass getwc als Makro implementiert werden
 * darf, das den Parameter mehrmals auswertet
 *
 * @see fgetwc
 */
#define getwc(stream) fgetwc(stream)

/**
 * Breites Zeichen von der Standardeingabe lesen.
 *
 * @see fgetwc
 */
wint_t getwchar(void);


/**
 * Ein breites Zeichen in eine Datei schreiben, dabei wird das Zeichen
 * entsprechend codiert.
 *
 * @param wc     Das zu schreibende Zeichen
 * @param stream Die geoeffnete Datei
 *
 * @return Das geschriebene Zeichen oder WEOF im Fehlerfall
 */
wint_t fputwc(wchar_t wc, FILE* stream);

/**
 * Wie fputwc, mit dem Unterschied, dass putwc als Makro implementiert werden
 * darf, das seine Parameter mehrmals auswertet.
 *
 * @see fputwc
 */
#define putwc(wc, stream) fputwc(wc, stream)

/**
 * Breites Zeichen auf die Standardausgabe schreiben
 *
 * @see fputwc
 * @param wc Das zu schreibende Zeichen
 *
 * @return Das geschriebene Zeichen oder WEOF im Fehlerfall
 */
wint_t putwchar(wchar_t wc);

#endif //ifndef _WCHAR_H_

