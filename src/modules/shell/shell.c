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
#include "dirent.h"
#include "lostio.h"
#define _USE_START_
#include "init.h"

#define COMMAND_BUFFER_SIZE 255


extern const char* fortune[];
byte fortune_id;
byte fortune_step;

FILE*   keyboard_resource;
char    shell_command_buffer[COMMAND_BUFFER_SIZE];
char    shell_current_dir[255];

char    keyboard_read_char();
void    shell_read_command();
bool    shell_check_command(const char* cmd);
char*   shell_get_absolute_path(char* path);

int main(int argc, char* argv[])
{
    int i;
    for(i=0; i < 10; i++)
    {
        yield();
    }

    keyboard_resource = fopen("keyboard:/ascii", "r");
    
    //Wenn die Tastatur nicht geoeffnet werden konnte wird abgebrochen
    if(keyboard_resource == NULL)
    {
        puts("[  LOSH  ] Konnte keyboard:/ascii nich oeffnen");
        return -1;
    }
    
    memcpy(shell_current_dir, "floppy:/devices/fd0|fat:/", strlen("floppy:/devices/fd0|fat:/") + 1);
    
    FILE* secs_res = fopen("cmos:/time/seconds", "r");
    fread(&fortune_id, 1, 1, secs_res);
    fclose(secs_res);
    fortune_step = fortune_step % 8 + 1;

    while(TRUE)
    {
        shell_read_command();
        
        if(shell_check_command("version"))
        {
            puts("Lost Shell V0.1");
        }
        if(shell_check_command("help"))
        {
            puts("Verfuegbare Befehle:");
            puts("  pwd            Zeigt den aktuellen Verzeichnisnamen an");
            puts("  ls             Zeigt den Verzeichnisinhalt an");
            puts("  cd <Pfad>      Wechselt das Verzeichnis");
            puts("  cat <Pfad>     Zeigt den Inhalt der Datei an");
            puts("  bincat <Pfad>  Zeigt den Inhalt der Datei als einzelne Bytes an");
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
                        printf("       '%s'\n", direntry->name);
                    }
                    else
                    {
                        printf(" [DIR] '%s'\n", direntry->name);
                    }

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
            }
        }
        else if(shell_check_command("date"))
        {
			FILE* weekday_file = fopen("cmos:/date/weekday", "r");
            FILE* day_file = fopen("cmos:/date/day", "r");
			FILE* month_file = fopen("cmos:/date/month", "r");
			FILE* century_file = fopen("cmos:/date/century", "r");
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

            if(century_file == NULL)
            {
                puts("Konnte 'cmos:/date/century' nicht zum lesen oeffnen!");
				error = TRUE;
            }


            if(year_file == NULL)
            {
                puts("Konnte 'cmos:/date/year' nicht zum lesen oeffnen!");
				error = TRUE;
            }
			if(error)
			{
				fclose(weekday_file);
				fclose(day_file);
				fclose(month_file);
				fclose(century_file);
				fclose(year_file);
			}
			else
			{
                char* buffer;
                buffer = malloc(1);
				byte weekday = 0, day = 0, month = 0, century = 0, year = 0;

                fread(buffer, 1, 1, weekday_file);
				weekday = buffer[0];

                fread(buffer, 1, 1, day_file);
				day = buffer[0];

				fread(buffer, 1, 1, month_file);
				month = buffer[0];

				fread(buffer, 1, 1, year_file);
				year = buffer[0];

				fread(buffer, 1, 1, century_file);
				century = buffer[0];

				fclose(weekday_file);
                fclose(day_file);
				fclose(month_file);
				fclose(year_file);
				fclose(century_file);

				switch(weekday) {
					case 1: printf("Sonntag, ");
						break;
					case 2: printf("Montag, ");
						break;
					case 3: printf("Dienstag, ");
						break;
					case 4: printf("Mittwoch, ");
						break;
					case 5: printf("Donnerstag, ");
						break;
					case 6: printf("Freitag, ");
						break;
					case 7: printf("Samstag, ");
						break;
					default: puts("BUG im CMOS Modul: Ungueltiger Wochentag");
				}

				printf("%02x", day);
				printf(".");
				printf("%02x", month);
				printf(".");
				printf("%02x", century);
				printf("%02x", year);
				printf("\n");
            }
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

			if(error)
			{
				fclose(seconds_file);
				fclose(minutes_file);
				fclose(hours_file);
			}
            else
            {
                char* buffer;
                buffer = malloc(1);
				byte seconds = 0, minutes = 0, hours = 0;
                
                fread(buffer, 1, 1, seconds_file);
				seconds = buffer[0];
				fread(buffer, 1, 1, minutes_file);
				minutes = buffer[0];
				fread(buffer, 1, 1, hours_file);
				hours = buffer[0];

                fclose(seconds_file);
				fclose(minutes_file);
				fclose(hours_file);

				printf("%02x", hours);
				printf(":");
				printf("%02x", minutes);
				printf(":");
				printf("%02x", seconds);
				printf("\n");
            }
        }
        else if(shell_check_command("pagefault"))
        {
            *((dword*) 0x42) = 0x42;
        }
        else if(shell_check_command("fortune"))
        {
            puts(fortune[fortune_id]);
            fortune_id += fortune_step;
        }
        else if(shell_check_command("start"))
        {
            char* start_args = strstr(shell_command_buffer, " ");

            if((start_args == NULL) || (*(++start_args) == 0))
            {
                puts("bincat: zu wenige Argumente");
            }
            else
            {
                char* start_path = shell_get_absolute_path(start_args);
                char* message = malloc(strlen(start_path) + 9);
                
                memcpy(message, "LOADELF ", 8);
                memcpy((void*) ((dword)message + 8), start_path, strlen(start_path) +1);
                puts(message);
                
                send_message(1, 512, 0, strlen(message) + 1, message);
                free(message);
            }
            
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
    while(c == 0)
    {
        fread(&c, 1, 1, keyboard_resource);
    }
    return c;
}


///Prompt anzeigen und Shellkommando von der Tastatur einlesen
void shell_read_command()
{
    int i;
    char c;
    
    printf("%s # ", shell_current_dir);
    i=0;
    while(TRUE)
    {
        c = keyboard_read_char();
        
        
        if(c == '\b')
        {
            if(i > 0)
            {
                syscall_puts("\033[1D \033[1D");

                i--;
            }
        }
        else
        {
            printf("%c", c);
            
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

