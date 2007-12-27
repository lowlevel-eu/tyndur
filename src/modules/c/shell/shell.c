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
#include "unistd.h"
#include "string.h"
#include "rpc.h"
#include "io.h"
#include "dir.h"
#include "lostio.h"
#include "config.h"
#include <sleep.h>

#include <readline/readline.h>
#include <readline/history.h>

#define _USE_START_
#include "init.h"


#include "shell.h"

#define COMMAND_BUFFER_SIZE 255


typedef struct shell_command_t {
    const char* name;
    int (*handler)(int argc, char* argv[], const char* args);
} shell_command_t;


char    shell_command_buffer[COMMAND_BUFFER_SIZE];

char    keyboard_read_char();
void    shell_read_command();
bool    shell_check_command(const char* cmd);
int     shell_do_cmd(const char* cmdline);


shell_command_t shell_commands[] = {
    {"version",     &shell_command_version},
    {"help",        &shell_command_help},
    {"exit",        &shell_command_exit},
    {"start",       &shell_command_start},
    {"cd",          &shell_command_cd}
#ifdef CONFIG_SHELL_BUILTIN_ONLY
   ,{"bincat",      &shell_command_bincat},
    {"cat",         &shell_command_cat},
    {"date",        &shell_command_date},
    {"date",        &shell_command_date},
    {"echo",        &shell_command_echo},
    {"free",        &shell_command_free},
    {"ls",          &shell_command_ls},
    {"irc",         &shell_command_irc},
    {"mkdir",       &shell_command_mkdir},
    {"pipe",        &shell_command_pipe},
    {"ps",          &shell_command_ps},
    {"pstree",      &shell_command_pstree},
    {"pwd",         &shell_command_pwd},
    {"symlink",     &shell_command_symlink},
    {"dbg_st",      &shell_command_dbg_st},
    {"cp",          &shell_command_cp},
    {"ln",        &shell_command_ln},
    {"rm",      &shell_command_rm}
#endif
};


int main(int argc, char* argv[])
{
    qword timeout = get_tick_count() + 20000000;
    while((init_service_get("cmos") == 0) 
            || (init_service_get("fat") == 0) 
            || (init_service_get("console") == 0)
        )
    {
        yield();
        if(get_tick_count() > timeout)
        {
            if(init_service_get("cmos") == 0)
            {
                puts("[ SHELL ] Konnte den CMOS-Treiber nicht finden!");
            }

            
            if(init_service_get("console") == 0)
            {
              puts("[ SHELL ] Konnte den Konsolentreiber nicht finden!");
            }

            puts("[ SHELL ] Shell beendet!");

            return -1;
        }
    }
    
    // FIXME
    msleep(1000);
    stdio_init();

    //Wenn die Tastatur nicht geoeffnet werden konnte wird abgebrochen
    if(stdin == NULL)
    {
        puts("[  LOSH  ] Konnte console:/stdin nich oeffnen");
        return -1;
    }
    
    
    // Keine Argumente wurden uebergeben, also wird die shell interaktiv
    // aufgerufen.
    if (argc == 1) {
        while (TRUE) {
            shell_read_command();
            shell_do_cmd(shell_command_buffer);
        }
    } else if ((argc == 2) && (strcmp(argv[1], "--help") == 0)) {
        puts("Aufruf:");
        puts("  Interaktiv: sh");
        puts("  Skript ausfuehren: sh <Dateiname>");
        puts("  Einzelnen Befehl ausfuehren: sh -c <Befehl>");

    } else if (argc == 2) {
        // Wenn nur ein Argument uebergeben wird, wird das Argument als
        // Dateiname fuer ein Shell-Skript benutzt.
        char* path = io_get_absolute_path(argv[1]);
        FILE* script = fopen(path, "r");
        free(path);

        while (feof(script) == 0) {
            fgets(shell_command_buffer, COMMAND_BUFFER_SIZE, script);
            puts(shell_command_buffer);
            shell_do_cmd(shell_command_buffer);
        }

        fclose(script);
    } else if ((argc == 3) && (strcmp(argv[1], "-c") == 0)) {
        // Eine einzelne Zeile soll ausgefuehrt werden
        shell_do_cmd(argv[2]);
    }
 
    return 0;
}

///Prompt anzeigen und Shellkommando von der Tastatur einlesen
void shell_read_command()
{
    char* cwd = getcwd(NULL, 0);
    char* prompt;
    char* input;
    
    asprintf(&prompt, "%s # ", cwd);

    input = readline(prompt);
    if (input) {
        if (*input) {
            add_history(input);
        }

        strncpy(shell_command_buffer, input, COMMAND_BUFFER_SIZE);
        free(input);
    } else {
        // Bei EOF wird die Shell beendet
        strcpy(shell_command_buffer, "exit");
    }

    free(prompt);
    free(cwd);
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


/**
 * Testet ob der Befehl am Anfang der uebergebenen Kommandozeile steht.
 *
 * @param cmd Pointer auf die Befehlsbeschreibung.
 * @param cmdline Die eingegebene Kommandozeile.
 *
 * @return TRUE wenn der Befehl drin steht, sonst FALSE
 */
bool shell_match_command(const char* cmd, const char* cmdline)
{
    while (TRUE)
    {
        // Wenn das Ende des Befehlsnamens erreicht ist, muss in der
        // Kommandozeile auch ein Nullbyte oder ein Leerschlag stehen.
        if ((*cmd == '\0') && ((*cmdline == '\0') ||(*cmdline == ' '))) {
            return TRUE;
        } else if (*cmd != *cmdline) {
            return FALSE;
        }
        cmd++;
        cmdline++;
    }
    
    // Kommt nie vor.
    return FALSE;
}


int shell_do_cmd(const char* cmdstring)
{
    shell_command_t* command;
    dword i;
    // Die Liste mit den Befehlen durchsuchen. Das wird solange gemacht, bis
    // der NULL eintrag am Ende erreicht wird.
    for (i = 0; i < sizeof(shell_commands) / sizeof(shell_command_t); i++)
    {
        command = &shell_commands[i];
        if (shell_match_command(command->name, cmdstring) == TRUE) {
            char args[strlen(cmdstring) + 1];
            memcpy(args, cmdstring, strlen(cmdstring) + 1);

            int argc, pos;
            argc = 1;
            //Jetzt werden die Argumente gezaehlt.
            for (pos = 0; pos < strlen(args); pos++) {
                if (args[pos] == ' ') {
                    argc++;
                }
            }
    
            char* argv[argc];
            argv[0] = strtok(args, " ");
            for(pos = 1; pos < argc; pos++)
            {
                argv[pos] = strtok(NULL, " ");
            }
            
            return command->handler(argc, argv, cmdstring);
        }
    }
    return shell_command_default(0, 0, cmdstring);
}

