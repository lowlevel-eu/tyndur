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
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "rpc.h"
#include "io.h"
#include "dir.h"
#include "lostio.h"
#define _USE_START_
#include "init.h"

#define COMMAND_BUFFER_SIZE 255


extern const char* fortune[];


//FILE*   stdin;
char    shell_command_buffer[COMMAND_BUFFER_SIZE];
char    shell_current_dir[255];

char    keyboard_read_char();
void    shell_read_command();
bool    shell_check_command(const char* cmd);
char*   shell_get_absolute_path(char* path);

int main(int argc, char* argv[])
{
    qword timeout = get_tick_count() + 20000000;
    while((init_service_get("keyboard") == 0) || (init_service_get("floppy") == 0) || (init_service_get("cmos") == 0) || (init_service_get("fat") == 0) || (init_service_get("console") == 0))
    {
        yield();
        if(get_tick_count() > timeout)
        {
            if(init_service_get("keyboard") == 0)
            {
                puts("[ SHELL ] Konnte den Tastaturtreiber nicht finden!");
            }
            
            /*if(init_service_get("floppy") == 0)
            {
                puts("[ SHELL ] Konnte den Diskettentreiber nicht finden!");
            }*/

            if(init_service_get("cmos") == 0)
            {
                puts("[ SHELL ] Konnte den CMOS-Treiber nicht finden!");
            }

            /*if(init_service_get("fat") == 0)
            {
                puts("[ SHELL ] Konnte den FAT-Treiber nicht finden!");
            }*/
            
            if(init_service_get("console") == 0)
            {
              puts("[ SHELL ] Konnte den Konsolentreiber nicht finden!");
            }

            puts("[ SHELL ] Shell beendet!");

            return -1;
        }
    }
    
    stdin = fopen("console:/stdin", "r");
    stdout = fopen("console:/stdout", "r");
    setvbuf(stdout, malloc(128), _IOLBF, 128);

    //Wenn die Tastatur nicht geoeffnet werden konnte wird abgebrochen
    if(stdin == NULL)
    {
        puts("[  LOSH  ] Konnte console:/stdin nich oeffnen");
        return -1;
    }
    
    
    memcpy(shell_current_dir, "floppy:/devices/fd0|fat:/", strlen("floppy:/devices/fd0|fat:/") + 1);
    //memcpy(shell_current_dir, "cmos:/", strlen("cmos:/") + 1);

    
    while(TRUE)
    {
        shell_read_command();
        
        if(shell_check_command("version"))
        {
            puts("Lost Shell V0.1");
            puts("$Rev$");
        }
        else if(shell_check_command("help"))
        {
            puts("Verfuegbare Befehle:");
            puts("  pwd            Zeigt den aktuellen Verzeichnisnamen an");
            puts("  ls             Zeigt den Verzeichnisinhalt an");
            puts("  cd <Pfad>      Wechselt das Verzeichnis");
            puts("  mkdir <Pfad>   Erstellt das Verzeichnis");
            
            puts("  cat <Pfad>     Zeigt den Inhalt der Datei an");
            puts("  bincat <Pfad>  Zeigt den Inhalt der Datei als einzelne Bytes an");
            
            puts("  start <Pfad>   Startet das Programm an <Pfad>");

            puts("  free           Zeigt den Speicherverbrauch des Systems an");

            puts("  date           Zeigt das Datum an");
            puts("  time           Zeigt die Uhrzeit an");
        }
        else if(shell_check_command("pwd"))
        {
            puts(shell_current_dir);
        }
        else if(shell_check_command("ls"))
        {
            char* ls_path = shell_current_dir;
            io_resource_t* dir_res = directory_open(ls_path);

            if(dir_res != NULL)
            {
                io_direntry_t* direntry;

                while((direntry = directory_read(dir_res)))
                {
                    if(direntry->type == IO_DIRENTRY_FILE)
                    {
                        printf("       %s\n", direntry->name);
                    }
                    else
                    {
                        printf(" [DIR] %s\n", direntry->name);
                    }
                    free(direntry);
                }

                directory_close(dir_res);
            }
            else
            {
                printf("Konnte '%s' nicht zum lesen oeffnen!\n", ls_path);
            }
        }
        else if(shell_check_command("cd"))
        {
            char* cd_args = strstr(shell_command_buffer, " ");

            if((cd_args == NULL) || (*(++cd_args) == 0))
            {
                puts("cd: zu wenige Argumente");
            }
            else
            {
                char* cd_path = shell_get_absolute_path(cd_args);
                
                io_resource_t* dir_res = directory_open(cd_path);
                if(dir_res == NULL)
                {
                    printf("Konnte '%s' nicht zum lesen oeffnen!\n", cd_path);
                }
                else
                {
                    directory_close(dir_res);
                    memcpy(shell_current_dir, cd_path, strlen(cd_path) + 1);
                }

                free(cd_path);
            }
            
        }
        else if(shell_check_command("cat"))
        {
            char* cat_args = strstr(shell_command_buffer, " ");

            if((cat_args == NULL) || (*(++cat_args) == 0))
            {
                puts("cat: zu wenige Argumente");
            }
            else
            {
                char* cat_path = shell_get_absolute_path(cat_args);
                FILE* cat_file = fopen(cat_path, "r");

                if(cat_file == NULL)
                {
                    printf("Konnte '%s' nicht zum lesen oeffnen!\n", cat_path);
                }
                else
                {
                    char* buffer;
                    buffer = malloc(1025);
                    size_t size;
                    while(feof(cat_file) != EOF)
                    {
                        size = fread(buffer, 1, 1024, cat_file);
                        buffer[size] = 0;

                        printf("%s", buffer);
                    }

                    free(buffer);
                    fclose(cat_file);
                    printf("\n");
                }
                free(cat_path);
            }
        }
        else if(shell_check_command("bincat"))
        {
            char* bincat_args = strstr(shell_command_buffer, " ");

            if((bincat_args == NULL) || (*(++bincat_args) == 0))
            {
                puts("bincat: zu wenige Argumente");
            }
            else
            {
                char* bincat_path = shell_get_absolute_path(bincat_args);
                FILE* bincat_file = fopen(bincat_path, "r");

                if(bincat_file == NULL)
                {
                    printf("Konnte '%s' nicht zum lesen oeffnen!\n", bincat_path);
                }
                else
                {
                    char* buffer;
                    buffer = malloc(1024);
                    size_t size;
                    int n, m = 0;
                    while(feof(bincat_file) != EOF)
                    {
                        size = fread(buffer, 1, 1024, bincat_file);

                        printf("0x%08x: ", m);

                        for(n=0; n < size; n++)
                        {
                            if((n % 8) == 0)
                            {
                                m += 8;
                                printf("\n");
                                printf("0x%08x: ", m);
                            }
                            char low = ("0123456789abcdef")[buffer[n] % 16];
                            char high = ("0123456789abcdef")[(buffer[n] >> 4) % 16];

                            printf("0x%c%c ", high, low);
                        }
                    }

                    free(buffer);
                    fclose(bincat_file);
                }

                free(bincat_path);
           }
        }
        else if(shell_check_command("writetest"))
        {
            char* writetest_args = strstr(shell_command_buffer, " ");

            if((writetest_args == NULL) || (*(++writetest_args) == 0))
            {
                puts("writetest: zu wenige Argumente");
            }
            else
            {
                char* writetest_path = shell_get_absolute_path(writetest_args);
                puts("Oeffne die Datei...");
                FILE* writetest_file = fopen(writetest_path, "aw");
                free(writetest_path);

                puts("Schreibe 'Hello LOST!' in die Datei...");
                const char* hello = "Hello LOST!";
                fwrite(hello, 1, strlen(hello), writetest_file);
                
                puts("Schliesse die Datei wieder...");
                fclose(writetest_file);
                
            }
        }
        else if(shell_check_command("mkdir"))
        {
            char* mkdir_args = strstr(shell_command_buffer, " ");

            if((mkdir_args == NULL) || (*(++mkdir_args) == 0))
            {
                puts("mkdir: zu wenige Argumente");
            }
            else
            {
                char* mkdir_path = shell_get_absolute_path(mkdir_args);
                if(directory_create(mkdir_path) == FALSE)
                {
                    puts("Das Verzeichnis konnte nicht erstellt werden");
                }
                free(mkdir_path);
            }
        }
        else if(shell_check_command("date"))
        {
            FILE* weekday_file = fopen("cmos:/date/weekday", "r");
            FILE* day_file = fopen("cmos:/date/day", "r");
            FILE* month_file = fopen("cmos:/date/month", "r");
            FILE* year_file = fopen("cmos:/date/year", "r");

            bool error = FALSE;

            if(weekday_file == NULL)
            {
                puts("Konnte 'cmos:/date/weekday' nicht zum lesen oeffnen!");
                error = TRUE;
            }

            
            if(day_file == NULL)
            {
                puts("Konnte 'cmos:/date/day' nicht zum lesen oeffnen!");
                error = TRUE;
            }

            if(month_file == NULL)
            {
                puts("Konnte 'cmos:/date/month' nicht zum lesen oeffnen!");
                error = TRUE;
            }

            if(year_file == NULL)
            {
                puts("Konnte 'cmos:/date/year' nicht zum lesen oeffnen!");
                error = TRUE;
            }

            if(error == FALSE)
            {
                byte weekday = 0, day = 0, month = 0;
                word year = 0;
                
                char* month_text[] = { "Januar", "Februar", "Maerz", "April", "Mai", "Juni", "Juli", "August", "November", "Dezember" };
                char* weekday_text[] = { "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag" };

                fread(&weekday, 1, 1, weekday_file);
                fread(&day, 1, 1, day_file);
                fread(&month, 1, 1, month_file);
                fread(&year, 1, 2, year_file);

                printf("%s, %d. %s %d\n", weekday_text[weekday], day, month_text[month -1], year);
            }

            fclose(weekday_file);
            fclose(day_file);
            fclose(month_file);
            fclose(year_file);
        }
        else if(shell_check_command("time"))
        {
            FILE* seconds_file = fopen("cmos:/time/seconds", "r");
            FILE* minutes_file = fopen("cmos:/time/minutes", "r");
            FILE* hours_file = fopen("cmos:/time/hours", "r");

            bool error = FALSE;
            
            if(seconds_file == NULL)
            {
                puts("Konnte 'cmos:/time/seconds' nicht zum lesen oeffnen!\n");
                error = TRUE;
            }

            if(minutes_file == NULL)
            {
                puts("Konnte 'cmos:/time/minutes' nicht zum lesen oeffnen!\n");
                error = TRUE;
            }

            if(hours_file == NULL)
            {
                puts("Konnte 'cmos:/time/hours' nicht zum lesen oeffnen!\n");
                error = TRUE;
            }


            if(error == FALSE)
            {
                byte seconds = 0, minutes = 0, hours = 0;
                
                fread(&seconds, 1, 1, seconds_file);
                fread(&minutes, 1, 1, minutes_file);
                fread(&hours, 1, 1, hours_file);

                printf("%02d:%02d:%02d\n", hours, minutes, seconds);
            }

            fclose(seconds_file);
            fclose(minutes_file);
            fclose(hours_file);
        }
        else if(shell_check_command("pagefault"))
        {
            *((dword*) 0x42) = 0x42;
        }
        else if(shell_check_command("free"))
        {
            memory_info_t mem_info = memory_info();

            puts("Speicherinformationen:");
            printf("  Gesamt: %d KB  =  %d MB\n", mem_info.total / 1024, mem_info.total / 1024 / 1024);
            printf("  Benutzt: %d KB  =  %d MB\n", (mem_info.total - mem_info.free) / 1024,  (mem_info.total - mem_info.free) / 1024 / 1024);
            printf("  Frei: %d KB  =  %d MB\n", mem_info.free / 1024, mem_info.free / 1024 / 1024);
        }
        else if(shell_check_command("start"))
        {
            char* start_args = strstr(shell_command_buffer, " ");

            if((start_args == NULL) || (*(++start_args) == 0))
            {
                puts("start: zu wenige Argumente");
            }
            else
            {
                char* start_path = shell_get_absolute_path(start_args);
                char* message = malloc(strlen(start_path) + 9);
                
                memcpy(message, "LOADELF ", 8);
                memcpy((void*) ((dword)message + 8), start_path, strlen(start_path) +1);
                
                send_message(1, 512, 0, strlen(message) + 1, message);
                free(message);
                free(start_path);
            }
            
            //fclose(stdin);
        }
        else if(strlen(shell_command_buffer) == 0)
        {
            continue;
        }
        else
        {
            puts("Unbekannter Befehl");
        }
    }
    
    return 0;
}

///Einzelnes Zeichen von der Tastatur einlesen
char keyboard_read_char()
{
    char c = 0;
    while((c == 0) || (c == '\t'))
    {
        fread(&c, 1, 1, stdin);
    }
    return c;
}


///Prompt anzeigen und Shellkommando von der Tastatur einlesen
void shell_read_command()
{
    int i;
    char c;
    
    printf("%s # ", shell_current_dir);
    fflush(stdout);
    i=0;
    while(TRUE)
    {
        c = keyboard_read_char();
        
        
        if(c == '\b')
        {
            if(i > 0)
            {
                printf("\033[1D \033[1D");
                fflush(stdout);
                i--;
            }
        }
        else
        {
            printf("%c", c);
            fflush(stdout);
            if((c != '\n') && (i < COMMAND_BUFFER_SIZE))
            {
                shell_command_buffer[i] = c;
            }
            else
            {
                shell_command_buffer[i] = 0;
                break;
            }
            i++;
        }
    }
}


bool shell_check_command(const char* cmd)
{
    if(strncmp(shell_command_buffer, cmd, strlen(cmd)) == 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


char* shell_get_absolute_path(char* path)
{
    char* buffer = malloc(strlen(path) + strlen(shell_current_dir) + 2);
    size_t cwd_len = strlen(shell_current_dir);
    
    //Ueberpruefen, ob ein relativer pfad vorliegt
    if(strcmp(path, "..") == 0)
    {
        //CWD kopieren
        memcpy(buffer, shell_current_dir, cwd_len + 1);

        int i = strlen(buffer) - 2;
        for(; i > 0; i--)
        {
            if(buffer[i] == '/')
            {
                if(buffer[i-1] == ':')
                {
                    i++;
                }
                buffer[i] = 0;
                break;
            }
        }
    }
    else if((strcmp(path, ".") == 0) && (strlen(path) == 1))
    {
        memcpy(buffer, shell_current_dir, cwd_len);
        buffer[cwd_len] = 0;
    }
    else if(strstr(path, ":/") == NULL)
    {
        //CWD kopieren
        memcpy(buffer, shell_current_dir, cwd_len);


        if(buffer[cwd_len-1] != '/')
        {
            buffer[cwd_len] = '/';
            memcpy((void*)((dword)buffer + cwd_len + 1), path, strlen(path) + 1);
        }
        else
        {
            memcpy((void*)((dword)buffer + cwd_len), path, strlen(path) + 1);
        }
    }
    else
    {
        memcpy(buffer, path, strlen(path) + 1);
    }
    return buffer;
}

