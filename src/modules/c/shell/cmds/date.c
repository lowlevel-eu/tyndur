#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "config.h"

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

        printf("%d. %s %d   %02d:%02d:%02d\n", /*weekday_text[weekday],*/ day,
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

