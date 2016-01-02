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
#include <string.h>
#include <lost/config.h>

static struct tm tm;

#ifndef CONFIG_LIBC_NO_STUBS
static char* asctime_buf = "Wed Jun 42 13:37:42 1337\n";
static char* ctime_string = "Wed Jun 42 13:37:42 1337\n";
#endif

enum {
    aJan, aFeb, aMar, aApr, aMay, aJun, aJul, aAug, aSep, aOct, aNov, aDec,
    Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec,
    aSun, aMon, aTue, aWed, aThu, aFri, aSat,
    Sun, Mon, Tue, Wed, Thu, Fri, Sat,
    Local_time, Local_date, DFL_FMT,
    AM, PM, DATE_FMT,
    LAST
};

static const char *__time[] = {
    "Jan","Feb","Mär","Apr","Mai","Jun","Jul","Aug","Sep","Okt","Nov","Dez",
    "Januar", "Februar", "März","April",
    "Mai","Juni", "Juli", "August", "September",
    "Oktober", "November", "Dezember",
    "So","Mo", "Di", "Mi","Do", "Fr","Sa",
    "Sonntag","Montag","Dienstag","Mittwoch", "Donnerstag","Freitag","Samstag",
    "%H:%M:%S","%m/%d/%y", "%a %b %d %H:%M:%S %Y",
    "AM", "PM", "%a %b %e %T %Z %Y", NULL
};

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
    static const int monat[2][13] = {
        { 0,31,59,90,120,151,181,212,243,273,304,334,365},
        { 0,31,60,91,121,152,182,213,244,274,305,335,366}
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
     *
     * FIXME Das ist natürlich falsch, wenn das Jahr durch 100, aber nicht
     * durch 400 teilbar ist. Aber ein 32-Bit-time_t reicht weder bis 1900
     * zurück noch bis 2100 in die Zukunft, insofern tut das für den Moment.
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

/**
 * Erzeugt einen String anhand eines Datumsformats aus einer bestimmten
 * Anzahl Zeichen.
 *
 * TODO: %z fuer Zeitzone, %c und %C fuer lokalisierte Datums- und Zeitformat,
 *       %x und %X fuer lokalisiertes Datums-, bzw. Zeitformat
 *
 * @param str Pointer auf einen String
 * @param maxs Maximal zu erzeugende Anzahl an Zeichen
 * @param datestr Pointer auf den Datumsformatstring
 * @param tm Struktur auf aktuelle Datums- und Zeitangaben
 *
 * @return 0 bei Misserfolg, ansonsten Anzahl erzeugter Zeichen
 **/
size_t strftime(char *str, size_t maxs, const char* datestr,
  const struct tm *tm)
{
    char i, c;
    size_t size = 0, n;
    const char* nstr;

    const int BUF_SIZE = 5;
    char buffer[BUF_SIZE];

    char dflcase[] = "%?";

    // Datumsformatstring parsen
    while ((c = *datestr++) != '\0') {
        if (c != '%') {
            if (++size >= maxs) {
                return 0;
            }

            *str++ = c;
            continue;
        }

        nstr = "";

        switch (*datestr++) {
            case '%':
                nstr = "%";
                break;

            case 'a':    // Abgekuerzter Wochentagsname
                nstr = (char *)__time[aSun + tm->tm_wday];
                break;

            case 'A':    // Wochentagsname
                nstr = (char *)__time[Sun + tm->tm_wday];
                break;

            case 'b':    // Abgekuerzter Montatsname
            case 'h':
                nstr = (char *)__time[aJan + tm->tm_mon];
                break;

            case 'B':    // Monatsname
                nstr = (char *)__time[Jan + tm->tm_mon];
                break;

            case 'd':    // Tagesnummer
                snprintf(buffer, BUF_SIZE, "%02i", tm->tm_mday);
                nstr = buffer;
                break;

            case 'D':
                if ((n = strftime(str, maxs-size, "%m/%d/%y", tm)) == 0) {
                    return 0;
                }

                str += n;
                size += n;
                break;

            case 'e':
                snprintf(buffer, BUF_SIZE, "%02i", tm->tm_mday);
                nstr = buffer;
                break;

            case 'H':    // 24-Stunden
                snprintf(buffer, BUF_SIZE, "%02i", tm->tm_hour);
                nstr = buffer;
                break;

            case 'I':    // 12-Stunden
                if ((i = tm->tm_hour % 12) == 0)
                    i = 12;
                snprintf(buffer, BUF_SIZE, "%02i", i);
                nstr = buffer;
                break;

            case 'j':    // Julianisches Datum
                snprintf(buffer, BUF_SIZE, "%03i", tm->tm_yday + 1);
                nstr = buffer;
                break;

            case 'm':    // Monats-Nummer
                snprintf(buffer, BUF_SIZE, "%02i", tm->tm_mon + 1);
                nstr = buffer;
                break;

            case 'M':    // Minute
                snprintf(buffer, BUF_SIZE, "%02i", tm->tm_min);
                nstr = buffer;
                break;

            case 'n':    // Newline
                nstr = "\n";
                break;

            case 'p':    // AM oder PM
                if (tm->tm_hour >= 12)
                    nstr = __time[PM];
                else
                    nstr = __time[AM];
                break;

            case 'r':
                if ((n = strftime(str, maxs-size, "%I:%M:%S %p", tm)) == 0) {
                    return 0;
                }

                str += n;
                size += n;
                break;

            case 'R':
                if ((n = strftime(str, maxs-size, "%H:%M", tm)) == 0) {
                    return 0;
                }

                str += n;
                size += n;
                break;

            case 'S':    // Sekunden
                snprintf(buffer, BUF_SIZE, "%02i", tm->tm_sec);
                nstr = buffer;
                break;

            case 't':    // Tabulator
                nstr = "\t";
                break;

            case 'T':
                if ((n = strftime(str, maxs-size, "%H:%M:%S", tm)) == 0) {
                    return 0;
                }

                str += n;
                size += n;
                break;

            case 'U':    // Wochennummer des Jahres, bei Sonntag als 1. Tag
                snprintf(buffer, BUF_SIZE, "%02i", (tm->tm_yday - tm->tm_wday + 7)/7);
                nstr = buffer;
                break;

            case 'w':    // Wochentagsnummer
                snprintf(buffer, BUF_SIZE, "%i", tm->tm_wday);
                nstr = buffer;
                break;

            case 'W':    // Wochennummer des Jahres, bei Montag als 1. Tag
                i = ((tm->tm_wday + 6) % 7);
                snprintf(buffer, BUF_SIZE, "%02i", (tm->tm_yday + i)/7);
                nstr = buffer;
                break;


            case 'y':    // Jahr in Form von "xx"
                snprintf(buffer, BUF_SIZE, "%02i", tm->tm_year % 100);
                nstr = buffer;
                break;

            case 'Y':    // Jahr in Form von "xxxx"
                snprintf(buffer, BUF_SIZE, "%4i", 1900 + tm->tm_year);
                nstr = buffer;
                break;

            default:
                dflcase[1] = *(datestr - 1);
                nstr = (char *)dflcase;
                break;
        }

        n = strlen(nstr);

        if ((size += n) >= maxs) {
            return 0;
        }

        strcpy(str, nstr);
        str += n;
    }

    *str = '\0';
    return size;
}

