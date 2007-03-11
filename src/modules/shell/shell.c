#include "types.h"
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "rpc.h"
#include "io.h"
#include "dirent.h"
#define _USE_START_
#include "init.h"

#define COMMAND_BUFFER_SIZE 255

FILE*   keyboard_resource;
char    shell_command_buffer[COMMAND_BUFFER_SIZE];
char    shell_current_dir[255];

char keyboard_read_char();
void shell_read_command();
bool shell_check_command(const char* cmd);

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
    
    memcpy(shell_current_dir, "keyboard:/", strlen("keyboard:/") + 1);
    
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
                        printf("       %s\n", direntry->name);
                    }
                    else
                    {
                        printf(" [DIR] %s\n", direntry->name);
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
            char* cd_path = strstr(shell_command_buffer, " ");
            
            if((cd_path == NULL) || (*(++cd_path) == 0))
            {
                puts("cd: zu wenige Argumente");
            }
            else
            {
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
            }
            
        }
        else if(shell_check_command("cat"))
        {
            char* cat_path = strstr(shell_command_buffer, " ");

            if((cat_path == NULL) || (*(++cat_path) == 0))
            {
                puts("cat: zu wenige Argumente");
            }
            else
            {
                FILE* cat_file = fopen(cat_path, "r");

                if(cat_file == NULL)
                {
                    printf("Konnte '%s' nicht zum lesen oeffnen!\n", cat_path);
                }
                else
                {
                    char* buffer;
                    buffer = malloc(33);
                    size_t size;
                    while(feof(cat_file) != EOF)
                    {
                        size = fread(buffer, 1, 32, cat_file);
                        buffer[size] = 0;

                        printf("%s", buffer);
                    }

                    free(buffer);
                    fclose(cat_file);
                }
            }
        }
        else if(shell_check_command("bincat"))
        {
            char* bincat_path = strstr(shell_command_buffer, " ");

            if((bincat_path == NULL) || (*(++bincat_path) == 0))
            {
                puts("bincat: zu wenige Argumente");
            }
            else
            {
                FILE* bincat_file = fopen(bincat_path, "r");

                if(bincat_file == NULL)
                {
                    printf("Konnte '%s' nicht zum lesen oeffnen!\n", bincat_path);
                }
                else
                {
                    char* buffer;
                    buffer = malloc(8);
                    size_t size;
                    int n, m = 0;
                    while(feof(bincat_file) != EOF)
                    {
                        size = fread(buffer, 1, 8, bincat_file);

                        printf("0x%08x: ", m);

                        for(n=0; n < size; n++)
                        {
                            printf("0x%02x ", buffer[n]);
                        }

                        m += size;
                        printf("\n");
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
                puts("Konnte 'cmos:/date/weekday' nicht zum lesen oeffnen!\n");
				error = TRUE;
            }

			
            if(day_file == NULL)
            {
                puts("Konnte 'cmos:/date/day' nicht zum lesen oeffnen!\n");
				error = TRUE;
            }

            if(month_file == NULL)
            {
                puts("Konnte 'cmos:/date/month' nicht zum lesen oeffnen!\n");
				error = TRUE;
            }

            if(century_file == NULL)
            {
                puts("Konnte 'cmos:/date/century' nicht zum lesen oeffnen!\n");
				error = TRUE;
            }


            if(year_file == NULL)
            {
                puts("Konnte 'cmos:/date/year' nicht zum lesen oeffnen!\n");
				error = TRUE;
            }

            if(!error)
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
					case 1: puts("Sonntag, ");
						break;
					case 2: puts("Montag, ");
						break;
					case 3: puts("Dienstag, ");
						break;
					case 4: puts("Mittwoch, ");
						break;
					case 5: puts("Donnerstag, ");
						break;
					case 6: puts("Freitag, ");
						break;
					case 7: puts("Samstag, ");
						break;
					default: puts("BUG im CMOS Modul");
				}

				if (day < 16)
				{
					printf("0%x", day);
				}
				else
				{
					printf("%x", day);
				}

				printf(".");

				if (month < 16)
				{
					printf("0%x", month);
				}
				else
				{
					printf("%x", month);
				}

				printf(".");

				if (century < 16)
				{
					printf("0%x", century);
				}
				else
				{
					printf("%x", century);
				}

				if (year < 16)
				{
					printf("0%x", year);
				}
				else
				{
					printf("%x", year);
				}

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

            if(!error)
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


				if (hours < 16)
				{
					printf("0%x", hours);
				}
				else
				{
					printf("%x", hours);
				}

				printf(":");

				if (minutes < 16)
				{
					printf("0%x", minutes);
				}
				else
				{
					printf("%x", minutes);
				}

				printf(":");

				if (seconds < 16)
				{
					printf("0%x", seconds);
				}
				else
				{
					printf("%x", seconds);
				}

				printf("\n");
            }
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
