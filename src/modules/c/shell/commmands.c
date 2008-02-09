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
#include "shell.h"
#include "dirent.h"
#include <sys/wait.h>
#include "init.h"


//Hier koennen die Debug-Nachrichten aktiviert werden
#define DEBUG 0

#if DEBUG == 1
    #define DEBUG_MSG(s) printf("[ SHELL ] debug: %s() '%s'\n",__FUNCTION__,s)
    #define DEBUG_MSG1(x, y) \
        printf("[ SHELL ] debug: %s() '", __FUNCTION__); \
        printf(x, y); \
        printf("'\n");
    #define DEBUG_MSG2(x, y, z) \
        printf("[ SHELL ] debug: %s() '", __FUNCTION__); \
        printf(x, y, z); \
        printf("'\n");
#else
    #define DEBUG_MSG(s)
    #define DEBUG_MSG1(s,x)
    #define DEBUG_MSG2(s,x,y)
#endif


int shell_start_path_app(const char* cmd, const char* path, bool wait)
{
    const char* bindir_path = path;
    DIR* bindir = opendir(bindir_path);
    if(bindir == NULL)
    {
        DEBUG_MSG1("Konnte das Applikationsverzeichnis '%s' nicht finden", 
            bindir_path);
        return -1;
    }
    struct dirent* dir_entry;
    
    do {
        dir_entry = readdir(bindir);
        if (dir_entry == NULL) {
            break;
        }
        if (shell_match_command(dir_entry->d_name, cmd) == TRUE) {
            // Hier wird nach Leerschlaegen gesucht, um festzustellen, ob dem
            // Befehl Argumente uebegeben werden sollen.
            char* start_opts = strchr(cmd, ' ');
            if (start_opts != NULL) {
                // Falls Argumente uebergeben werden sollen, wird der
                // Leerschlag, der die Argumente vom Programmnamen trennt durch
                // ein Nullbyte ersetzt, da das die Verarbeitung des
                // Programmnamens erleichtert.
                *start_opts = '\0';
                
                // Jetzt werden noch weitere Leerschlaege uebersprungen, da die
                // nicht erwuenscht sind.
                while (*(++start_opts) == ' ');
                start_opts--;
            }
            
            // Speicher fuer den neuen Pfad. Besteht aus: Dem Verzeichnis mit
            // den Anwendungen, einem Slash, dem Anwendungsnamen und einem
            // abschliessenden Nullbyte. Die seltsame Darstellung dient nur der
            // besseren Lesbarkeit, und wird hoffentlich vom Compiler
            // wegoptimiert ;-).            
            char start_path[strlen(bindir_path) + 1 + strlen(cmd) + 1];
            bool insert_slash = (bindir_path[strlen(bindir_path)-1] != '/');

            memcpy(start_path, bindir_path, strlen(bindir_path));            
            if (insert_slash) {
                start_path[strlen(bindir_path)] = '/';
                memcpy(start_path + strlen(bindir_path) + 1, 
                    cmd, strlen(cmd) + 1);
            } else {
                memcpy(start_path + strlen(bindir_path), cmd, strlen(cmd) + 1);
            }
            

            char* message;

            // FIXME: Hier sollten eigentlich keine mallocs() noetig sein...
            if (start_opts == NULL) {
                // Wenn keine Kommandozeilenargumente an das Programm
                // uebergeben werden sollen, muss nur der Pfad des Programms an
                // init geschickt werden.
                message = malloc(strlen(start_path) + 1);
                //memcpy(message, "LOADELF ", 8);
                memcpy((void*) ((dword)message/* + 8*/), start_path, 
                    strlen(start_path) + 1);
            } else {
                // Da vorher der Leerschlag durch ein Nullbyte ersetzt wurde,
                // wird das hier wieder korrigiert, damit die Argumente einfach
                // kopiert werden koennen.
                *start_opts = ' ';

                message = malloc(strlen(start_path) + 1 + 
                    strlen(start_opts));
                
                memcpy((void*) ((dword)message /*+ 8*/), start_path, 
                    strlen(start_path));
                memcpy((void*) ((dword)message /*+ 8 */+ 
                    strlen(start_path)), start_opts, 
                    strlen(start_opts) + 1);
            }
            
            // Jetzt wird der RPC an Init geschickt. Hier sollte noch irgend
            // was hin, um zu warten bis der Prozess beendet wurde.
            //send_message(1, 512, 0, strlen(message) + 1, message);
            DEBUG_MSG("Starte das Programm");
            pid_t pid = init_execute(message);
            
            // Fehler ist aufgetreten
            if (pid == 0) {
                free(message);
                closedir(bindir);
                return -1;
            }

            // Wenn es gewuenscht wurde, wird jetzt gewartet, bis der Prozess
            // beendet wird.
            if (wait == TRUE) {
                int status;
                while (waitpid(pid, &status, 0) != pid);
            }

            // FIXME: s.o.
            free(message);
            closedir(bindir);
            return 0;
        }
        
        free(dir_entry);

    } while(dir_entry != NULL);

    closedir(bindir);
    return -1;
}

/**
 * Wird aufgerufen, wenn keiner der internen Befehle passt. Dabei wird das
 * apps-Verzeichnis durchsucht.
 */
int shell_command_default(int argc, char* argv[], const char* cmd)
{
    // Leere Kommandozeilen sollen keine Fehlermeldungen bringen
    if (strlen(cmd) == 0) {
        return 0;
    }
    
    int result;
    const char* path = getenv("PATH");

    const char* cwd = getcwd(NULL, 0);
    result = shell_start_path_app(cmd, cwd, TRUE);
    free((char*) cwd);
    
    // FIXME: Das ist so eigentlich nicht richtig...
    if (result == 0) {
        return result;
    } else {
        result = shell_start_path_app(cmd, path, TRUE);
        if (result == 0) {
            return 0;
        }
    }
    puts("Befehl wurde nicht gefunden!");

    
    return 1;
}

/**
 * Eine ausfuerbare Datei im aktuellen Verzeichnis oder in PATH starten.
 */
int shell_command_start(int argc, char* argv[], const char* cmd)
{
    DEBUG_MSG("start");
    
    char* space = strchr((char*) cmd, ' ');
    if (space == NULL) {
        DEBUG_MSG("space == NULL");
        return 0;
    }

    cmd = space + 1;

    int result;
    const char* path = getenv("PATH");
    char* cwd = getcwd(NULL, 0);
    
    DEBUG_MSG("Starte aus dem aktuellen Verzeichnis");
    result = shell_start_path_app(cmd, cwd, FALSE);
    free(cwd);
    
    // FIXME: Das ist so eigentlich nicht richtig...
    if (result == 0) {
        DEBUG_MSG("result == 0");
        return result;
    } else {
        DEBUG_MSG("Starte aus dem Pfad");
        result = shell_start_path_app(cmd, path, FALSE);
        if (result == 0) {
            DEBUG_MSG("result == 0");
            return result;
        }
    }
    puts("Befehl wurde nicht gefunden!");
    
    return 1;
}


/**
 *
 */
int shell_command_help(int argc, char* argv[], const char* args)
{
    puts("Verfuegbare Befehle:");
    puts("  echo <Text>    Zeigt <Text> an");
    puts("  date           Zeigt das Datum und die Uhrzeit an");

    puts("  pwd            Zeigt den aktuellen Verzeichnisnamen an");
    puts("  ls             Zeigt den Verzeichnisinhalt an");
    puts("  cd <Pfad>      Wechselt das Verzeichnis");
    puts("  mkdir <Pfad>   Erstellt das Verzeichnis");
    
    puts("  cat <Pfad>     Zeigt den Inhalt der Datei an");
    puts("  bincat <Pfad>  Zeigt den Inhalt der Datei als einzelne Bytes an");
    
    puts("  start <Pfad>   Startet das Programm an <Pfad>");

    puts("  free           Zeigt den Speicherverbrauch des Systems an");
    puts("  ps             Zeigt eine Liste mit allen Prozessen an");
    puts("  pstree         Zeigt die Prozesshierarchie als Baum an");
    puts("  dbg_st <PID>   Zeigt einen Stack Backtrace des Protesses <PID>"
        " an");
    puts("  exit           Beendet die Shell");

    return 0;
}

/**
 *
 */
int shell_command_version(int argc, char* argv[], const char* args)
{
    puts("Lost Shell V0.1");
    puts("$Rev$");
    return 0;
}

/**
 *
 */
int shell_command_exit(int argc, char* argv[], const char* args)
{
    exit(0);
    return 0;
}

int shell_command_cd(int argc, char* argv[], const char* args)
{
    char* cd_args = strchr(args, ' ');

    if (cd_args == NULL) {
        puts("Aufruf: cd <Verzeichnis>");
        return -1;
    } else {
        cd_args++;
        char* path = io_get_absolute_path(cd_args);

        DIR* dir = opendir(path);
        if (dir == NULL) {
            printf("Wechseln in das Verzeichnis '%s' nicht moeglich\n", path);
        } else {
            chdir(path);
        }
        free(path);
    }
    return 0;
}

