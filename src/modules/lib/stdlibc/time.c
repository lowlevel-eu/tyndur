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

#include <time.h>
#include <stdio.h>
#include <lost/config.h>

#ifndef CONFIG_LIBC_NO_STUBS
static struct tm tm = {
    .tm_wday = 3,
    .tm_mday = 42,
    .tm_mon = 6,
    .tm_year = 1337,
    .tm_yday = 1337,

    .tm_hour = 13,
    .tm_min = 37,
    .tm_sec = 42
};
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

#ifndef CONFIG_LIBC_NO_STUBS
/**
 * TODO
 **/
struct tm* localtime(const time_t* time)
{
    // TODO
    return &tm;
}

/**
 * Timestamp in tm-Struktur einfuellen
 *
 * @param time_ptr Pointer auf den Timestamp
 *
 * @return Pointer auf die interne statisch allozierte tm-Struktur. Diese kann
 *          bei jedem Aufruf einer Zeitfunktion ueberschrieben werden!
 */
struct tm* gmtime(const time_t* time_ptr)
{
    // TODO
    return &tm;
}

/**
 * TODO
 **/
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

#endif

