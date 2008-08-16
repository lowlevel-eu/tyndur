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

#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include <lost/config.h>

void date_display_usage();

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_date(int argc, char* argv[], const char* args)
#else
    #define _USE_START_
    #include "init.h"
    int main(int argc, char* argv[])
#endif
{
    if (argc > 1) {
        date_display_usage();
        return -1;
    }
    

    // FIXME: Hier sollte cmos ein sauber formatiertes Datum zur Verfuegung
    //stellen
    FILE* weekday_file = fopen("cmos:/date/weekday", "r");
    FILE* day_file = fopen("cmos:/date/day", "r");
    FILE* month_file = fopen("cmos:/date/month", "r");
    FILE* year_file = fopen("cmos:/date/year", "r");
    FILE* seconds_file = fopen("cmos:/time/seconds", "r");
    FILE* minutes_file = fopen("cmos:/time/minutes", "r");
    FILE* hours_file = fopen("cmos:/time/hours", "r");

    bool error = FALSE;

    if (weekday_file == NULL) {
        puts("Konnte 'cmos:/date/weekday' nicht zum lesen oeffnen!");
        error = TRUE;
    }

    if (day_file == NULL) {
        puts("Konnte 'cmos:/date/day' nicht zum lesen oeffnen!");
        error = TRUE;
    }
    
    if (month_file == NULL) {
        puts("Konnte 'cmos:/date/month' nicht zum lesen oeffnen!");
        error = TRUE;
    }

    if (year_file == NULL) {
        puts("Konnte 'cmos:/date/year' nicht zum lesen oeffnen!");
        error = TRUE;
    }
    
    if (seconds_file == NULL) {
        puts("Konnte 'cmos:/time/seconds' nicht zum lesen oeffnen!\n");
        error = TRUE;
    }

    if (minutes_file == NULL) {
        puts("Konnte 'cmos:/time/minutes' nicht zum lesen oeffnen!\n");
        error = TRUE;
    }

    if (hours_file == NULL) {
        puts("Konnte 'cmos:/time/hours' nicht zum lesen oeffnen!\n");
        error = TRUE;
    }

    if (error == FALSE) {
        byte weekday = 0, day = 0, month = 0, seconds = 0, minutes = 0,
            hours = 0;
        word year = 0;
                
        char* month_text[] = { "Januar", "Februar", "Maerz", "April", "Mai",
            "Juni", "Juli", "August", "September", "Oktober", "November", 
            "Dezember" };
        char* weekday_text[] = { "Sonntag", "Montag", "Dienstag", "Mittwoch",
            "Donnerstag", "Freitag", "Samstag" };

        fread(&weekday, 1, 1, weekday_file);
        fread(&day, 1, 1, day_file);
        fread(&month, 1, 1, month_file);
        fread(&year, 1, 2, year_file);
        fread(&seconds, 1, 2, seconds_file);
        fread(&minutes, 1, 2, minutes_file);
        fread(&hours, 1, 2, hours_file);

        printf("%s, %d. %s %d   %02d:%02d:%02d\n", weekday_text[weekday], day,
            month_text[month -1], year, hours, minutes, seconds);
    }

    fclose(weekday_file);
    fclose(day_file);
    fclose(month_file);
    fclose(year_file);
    fclose(seconds_file);
    fclose(minutes_file);
    fclose(hours_file);
    
    if (error == FALSE) {
        return EXIT_SUCCESS;
    } else {
        return -1;
    }
}

void date_display_usage()
{
    puts("\nAufruf: date");
}

