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

#include <time.h>
#include <stdio.h>
#include <lost/config.h>

static struct tm tm;

#ifndef CONFIG_LIBC_NO_STUBS
static char* asctime_buf = "Wed Jun 42 13:37:42 1337\n";
static char* ctime_string = "Wed Jun 42 13:37:42 1337\n";
#endif

/**
 * Unix-Timestamp aus der aktuellen Zeit generieren.
 *
 * @return Unix-Timestamp
 */
time_t time(time_t* t)
{
    time_t result = 0;

    FILE* time_file = fopen("cmos:/unix_time", "r");
    if (time_file == NULL) {
        printf("time():Konnte 'cmos:/unix_time' nicht oeffnen. Ist das ");
        printf("cmos-Modul geladen?\n");
        return 0;
    }

    fread(&result, sizeof(time_t), 1, time_file);
    fclose(time_file);

    if (t != NULL) {
        *t = result;
    }
    return result;
}

/**
 * Timestamp, unter Bachtung der Zeitzone, in tm-Struktur einfuellen
 *
 * @param timer_ptr Pointer auf den Timestamp
 * @param result Pointer auf die tm-Struktur, die gefuellt werden soll.
 *
 * @return result
 */
struct tm* localtime_r(const time_t* time_ptr, struct tm* result)
{
    /* FIXME hier wird fest von UTC+1 ausgegangen */
    time_t ltime = *time_ptr + 60*60;
    return gmtime_r(&ltime, result);
}

/**
 * Timestamp, unter Bachtung der Zeitzone, in tm-Struktur einfuellen
 *
 * @param timer_ptr Pointer auf den Timestamp
 *
 * @return Pointer auf die interne statisch allozierte tm-Struktur. Diese kann
 *          bei jedem Aufruf einer Zeitfunktion ueberschrieben werden!
 */
struct tm* localtime(const time_t* time_ptr)
{
    return localtime_r(time_ptr, &tm);
}

/**
 * Timestamp in tm-Struktur einfuellen
 *
 * @param timer_ptr Pointer auf den Timestamp
 * @param result Pointer auf die tm-Struktur, die gefuellt werden soll.
 *
 * @return result
 */
struct tm* gmtime_r(const time_t* timer_ptr, struct tm* result)
{
    static const int monat[2][12] = {
        { 0,31,59,90,120,151,181,212,243,273,304,334},
        { 0,31,60,91,121,152,182,213,244,274,305,335}
    };

    time_t time = *timer_ptr;
    int schaltjahr = 0;

    /* die Uhrzeit ist noch einfach */
    result->tm_sec = time % 60;
    time /= 60;
    result->tm_min = time % 60;
    time /= 60;
    result->tm_hour = time % 24;
    time /= 24;

    /* 01.01.70 wahr ein Donnerstag (4) */
    result->tm_wday = (time + 4) % 7;

    /* jetzt kommen die Schaltjahre ins Spiel */

    /* rebase auf 1969-01-01 00:00:00 UTC
     * dann ist jedes 4. Jahr ein Schaltjahr
     */
    time += 365;

    int tmp_years = 69 + 4 * (time / (4*365 + 1));
    time %= 4*365 + 1;

    if( (time / 365) > 2 ) {
        schaltjahr = 1;
        result->tm_year = tmp_years + 3;
        result->tm_yday = time - 3*365;
    } else {
        result->tm_year = tmp_years + (time / 365);
        result->tm_yday = time % 365;
    }

    /* den richtigen monat raussuchen */
    if( monat[schaltjahr][result->tm_yday / 30] > result->tm_yday ) {
        result->tm_mon = (result->tm_yday / 30) - 1;
    } else if( monat[schaltjahr][result->tm_yday / 30 + 1] > result->tm_yday ) {
        result->tm_mon = (result->tm_yday / 30);
    } else {
        result->tm_mon = (result->tm_yday / 30) + 1;
    }

    result->tm_mday = 1 + (result->tm_yday - monat[schaltjahr][result->tm_mon]);

    result->tm_isdst = -1;

    return result;
}

/**
 * Timestamp in tm-Struktur einfuellen
 *
 * @param timer_ptr Pointer auf den Timestamp
 *
 * @return Pointer auf die interne statisch allozierte tm-Struktur. Diese kann
 *          bei jedem Aufruf einer Zeitfunktion ueberschrieben werden!
 */
struct tm* gmtime(const time_t* timer_ptr)
{
    return gmtime_r(timer_ptr, &tm);
}


#ifndef CONFIG_LIBC_NO_STUBS

/**
 * TODO
 */
char* asctime(const struct tm* time)
{
    // TODO
    return asctime_buf;
}

/**
 * Datum und Zeit in einen String umwandeln
 *
 * @param time_ptr Pointer auf die Zeit
 *
 * @return Pointer auf den Buffer mit dem String. Statisch alloziert, kann von
 *          weiteren aufrufen ueberschrieben werden.
 */
char* ctime(const time_t* time_ptr)
{
    // TODO
    return ctime_string;
}

/**
 * Unix-Timestamp aus einer tm-Struktur erzeugen
 */
time_t mktime(struct tm* time_ptr)
{
    return 0;
}

#endif

