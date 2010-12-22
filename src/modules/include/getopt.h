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

#ifndef _GETOPT_H_

#ifndef __unistd_only
#define _GETOPT_H_
#endif

/** Pointer auf den aktuellen argumentstring zur aktuellen Option oder NULL */
extern char*    optarg;

/** Index des naechsten zu Verarbeitenden Elements in argv */
extern int      optind;

/** Aktuelles Zeichen, auch wenn der getopt-Aufruf nicht erfolgreich war. */
extern int      optopt;


/** Wenn dieser Wert auf 0 gesetzt ist, gibt getopt keine Fehlermeldungen aus */
extern int      opterr;

/** Auf != 0 setzen damit getopt beim naechsten Aufruf vorne Anfaengt. */
extern int      optreset;


/**
 * Kommandozeilenoptionen in der Form "-x[arg]" verarbeiten, wobei x nur ein
 * nahezu beiliebiges Zeichen(nicht -+:) sein kann. Mit jedem Aufruf wird eine
 * Option verarbeitet. Um alle Optionen zu verarbeiten, muss getopt solange
 * aufgerufen werden, bis es -1 zurueckgibt.
 *
 * Bei Erfolg aktualisiert die Funktion den Pointer optarg so dass er auf den
 * Argumenttext zeigt, falls die aktuelle Option ein Argument nimmt, oder auf
 * NULL wenn nicht.
 *
 * @param argc Anzahl der Elemente in argv
 * @param argv Pointer auf ein Array mit den Parametern
 * @param optstring String der die akzeptierten Optionen beschreibt. Diesser
 *                  String besteht aus den Zeichen fuer die einzelnen Optionen,
 *                  die von einem Doppelpunkt gefolgt werden koennen, wenn ein
 *                  Argument folgen muss. In diesem Fall wird der Pointer optarg
 *                  nach dem Aufruf auf den Argumenttext zeigen.
 *
 * @return Die Verarbeitete Option bei Erfolg, im Fehlerfall ':' wenn das
 * Argument zu einer Option fehlt, '?' wenn eine ungueltige Option gefunden
 * wurde, oder -1 wenn die Optionen alle verarbeitet wurden.
 */
int getopt(int argc, char* const argv[], const char* optstring);

#ifndef __unistd_only

// Fuer den has_arg-Member in der option-struct
#define no_argument 0
#define required_argument 1
#define optional_argument 2

/**
 * Struktur die eine lange Option fuer getopt_long oder getopt_long_only
 * beschreibt
 */
struct option {
    /** Name der Option (der Text wie sie benutzt wird) */
    const char* name;

    /**
     * 0 oder no_argument wenn die Option kein Argument nimmt
     * 1 oder required_argument wenn die Option ein Argument haben muss
     * 2 oder optional_argument wenn die Option ein Argument haben kann, aber
     *                          nicht muss
     */
    int         has_arg;

    /**
     * Ist dieser Pointer != NULL, wird an dieser Speicherstelle der Wert val
     * abgelegt, wenn diese Option gefunden wird.
     */
    int*        flag;

    /**
     * Wert den die Funktionen zurueckgeben oder allenfalls ind *flag ablegen
     * */
    int         val;
};

/**
 * Diese Funktion arbeitet gleich wie getopt, akzeptiert aber auch lange
 * Optionen, die mit zwei Minuszeichen eingeleitet werden. Argumente koennen
 * ihnen entweder in der form --param arg oder --param=arg uebergeben werden.
 *
 * Die langen Optionen werden mit einem Array von struct option beschrieben.
 * Das letzte Element in diesem Array muss alle Member auf Null gesetzt haben.
 *
 * @param argc Anzahl der Elemente in argv
 * @param argv Pointer auf ein Array mit den Parametern
 * @param optstring Optstring wie bei getopt
 * @param longopts Array von struct option
 * @param longindex Wenn != NULL, wird in ihr der Index der Option abgelegt,
 *                  relativ zu longopts
 *
 * @return Wenn eine lange Option gefunden wurde, haeng das Resultat vom
 * flag-Member in der struct option ab. Ist er NULL wird der val-Member
 * zurueckgegeben, sonst wird der Wert des val-Members in diese Speicherstelle
 * geschrieben. Wenn eine kurze Option gefunden wurde, ist das Verhalten gleich
 * wie bei getopt, ebenfalls wenn keine gefunden wurde.
 */
int getopt_long(int argc, char* const argv[], const char* optstring,
    const struct option* longopts, int* longindex);

/**
 * Diese Funktion verhaelt sich aehnlich wie getopt_long. Der Unterschied ist,
 * dass getopt_long_only auch lange optionen akzeptiert, die mit nur einem
 * Bindestrich eingeleitet werden, also -option. Werden hier mehrere Buchstaben
 * nach einem Minus gefunden, die auf kein Element in longopts passen, werden
 * sie wie einzelne kurze Optionen verarbeitet.
 *
 * @param argc Anzahl der Elemente in argv
 * @param argv Pointer auf ein Array mit den Parametern
 * @param optstring Optstring wie bei getopt
 * @param longopts Array von struct option
 * @param longindex Wenn != NULL, wird in ihr der Index der Option abgelegt,
 *                  relativ zu longopts
 *
 * @return Gleich wie bei getopt_long.
 */
int getopt_long_only(int argc, char* const argv[], const char* optstring,
    const struct option* longopts, int* longindex);

#endif /* !defined(__unistd_only) */

#endif
