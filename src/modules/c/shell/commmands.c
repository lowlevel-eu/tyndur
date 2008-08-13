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

#include <types.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <rpc.h>
#include "shell.h"
#include <dirent.h>
#include <sys/wait.h>
#include <init.h>
#include <io.h>


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


int shell_start_path_app(const char* cmd, bool wait)
{
    DEBUG_MSG1("Starte das Programm %s", cmd);
    pid_t pid = init_execute(cmd);

    // Fehler ist aufgetreten
    if (pid == 0) {
        return -1;
    }

    // Wenn es gewuenscht wurde, wird jetzt gewartet, bis der Prozess beendet
    // wird.
    if (wait == TRUE) {
        int status;
        while (waitpid(pid, &status, 0) != pid);
    }
    return 0;
}

/**
 * Wird aufgerufen, wenn keiner der internen Befehle passt.
 */
int shell_command_default(int argc, char* argv[], const char* cmd)
{
    // Leere Kommandozeilen sollen keine Fehlermeldungen bringen
    if (strlen(cmd) == 0) {
        return 0;
    }
    
    if (shell_start_path_app(cmd, TRUE) == 0) {
        return 0;
    }

    puts("Befehl wurde nicht gefunden!");
    return -1;
}

/**
 * Eine ausfuerbare Datei im aktuellen Verzeichnis oder in PATH starten.
 */
int shell_command_start(int argc, char* argv[], const char* cmd)
{
    const char* program;
    DEBUG_MSG("start");

    program = strchr(cmd, ' ') + 1;
    if (program == NULL) {
        return -1;
    }

    if (shell_start_path_app(program, FALSE) == 0) {
        return 0;
    }   
    
    puts("Befehl wurde nicht gefunden!");
    return -1;
}


/**
 *
 */
int shell_command_help(int argc, char* argv[], const char* args)
{
    puts("Verfuegbare Befehle:");
    
    puts("  <Pfad>         Startet das Programm an <Pfad> im Vordergrund");
    puts("  start <Pfad>   Startet das Programm an <Pfad> im Hintergrund");
    puts("  echo <Text>    Zeigt <Text> an");
    puts("");
    puts("  pwd            Zeigt den aktuellen Verzeichnisnamen an");
    puts("  ls             Zeigt den Verzeichnisinhalt an");
    puts("  stat           Zeigt Informationen zur einer Datei an.");
    puts("  cd <Pfad>      Wechselt das Verzeichnis");
    puts("  mkdir <Pfad>   Erstellt das Verzeichnis");
    puts("");
    puts("  cp <Q> <Z>     Kopiert die Datei <Q> nach <Z>");
    puts("  ln <Q> <Z>     Erstellt einen auf <Q> verweisenden Link <Z>");
    puts("  symlink <Q><Z> Erstellt einen symbolischen Link <Z> auf <Q>");
    puts("  readlink <L>   Liest das Ziel des Symlinks <L> aus");
    puts("  rm <Pfad>      Loescht eine Datei");
    puts("");
    puts("  cat <Pfad>     Zeigt den Inhalt der Datei an");
    puts("  bincat <Pfad>  Zeigt den Inhalt der Datei als einzelne Bytes an");
    puts("  pipe <Pfad>    Ein-/Ausgabe auf einer Datei"
        " (z.B. TCP-Verbindung)");
    puts("");
    puts("  date           Zeigt das Datum und die Uhrzeit an");
    puts("  free           Zeigt den Speicherverbrauch des Systems an");
    puts("  ps             Zeigt eine Liste mit allen Prozessen an");
    puts("  pstree         Zeigt die Prozesshierarchie als Baum an");
    puts("  dbg_st <PID>   Zeigt einen Stack Backtrace des Protesses <PID>"
        " an");
    
    puts("  set            Setzt Umgebungsvariablen oder zeigt sie an");
    puts("  source <S>     Fuehrt das Skript <S> in der aktuellen Shell aus");
    puts("  sleep <S>      Wartet <S> Sekunden");

    puts("  exit           Beendet die Shell");
    puts("");
    puts("Diese Hilfe wird in den meisten Faellen nicht auf den Bildschirm"
        " passen. Um");
    puts("weiter oben stehende Befehle anzuzeigen, bitte Shift-BildAuf"
        " druecken.");
    puts("Shift-BildAb blaettert wieder nach unten. Andere Tasten springen"
        " zurueck an die");
    puts("aktuelle Position.");
    puts("");

    return 0;
}

/**
 *
 */
int shell_command_version(int argc, char* argv[], const char* args)
{
    puts("Lost Shell v0.2");
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

int shell_command_source(int argc, char* argv[], const char* args)
{
    const char* path;
    if (argc != 2) {
        puts("Aufruf: source <Pfad>");
        return -1;
    }

    path = argv[1];
    if (shell_script(path)) {
        return 0;
    } else {
        printf("Ausfuehren des Skripts '%s' fehlgeschlagen.\n", path);
        return -1;
    }
}

