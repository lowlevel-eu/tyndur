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
                    buffer = malloc(4096);
                    
                    size_t size = fread(buffer, 1, 4095, cat_file);
                    buffer[size] = 0;

                    puts(buffer);

                    free(buffer);

                    fclose(cat_file);
                }
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
