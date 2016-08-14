/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
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

#include <stdbool.h>
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
#include <env.h>

#define TMS_MODULE cmd
#include <tms.h>

#include <lost/config.h>


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


static int shell_start_path_app(char* argv[], bool wait)
{
    DEBUG_MSG1("Starte das Programm %s", argv[0]);
    pid_t pid;

    pid = init_execv(argv[0], (const char const**) argv);

    // Fehler ist aufgetreten
    if (pid == 0) {
        return -1;
    }

    // Wenn es gewuenscht wurde, wird jetzt gewartet, bis der Prozess beendet
    // wird.
    if (wait == true) {
        int status;
        while (waitpid(pid, &status, 0) != pid);
    }
    return 0;
}

/**
 * Wird aufgerufen, wenn keiner der internen Befehle passt.
 */
int shell_command_default(int argc, char* argv[])
{
    // Leere Kommandozeilen sollen keine Fehlermeldungen bringen
    if (argc == 0) {
        return 0;
    }

    if (shell_start_path_app(argv, true) == 0) {
        return 0;
    }

    puts(TMS(exec_not_found, "Befehl wurde nicht gefunden!"));
    return -1;
}

/**
 * Eine ausfuerbare Datei im aktuellen Verzeichnis oder in PATH starten.
 */
int shell_command_start(int argc, char* argv[])
{
    DEBUG_MSG("start");

    if (argc < 2) {
        printf(TMS(start_usage, "\nAufruf: start <Pfad> [Argumente ...]\n"));
        return -1;
    }

    if (shell_start_path_app(&argv[1], false) == 0) {
        return 0;
    }

    puts(TMS(start_not_found, "Datei nicht gefunden!"));
    return -1;
}


/**
 *
 */
int shell_command_help(int argc, char* argv[])
{
    char** thelp_args;
    int ret;

    // Erstmal versuchen, ob thelp installiert ist
    thelp_args = (char*[]) {
        "thelp",
        argc > 1 ? argv[1] : "tyndur",
        NULL,
    };
    ret = shell_start_path_app(thelp_args, true);
    if (ret == 0) {
        return 0;
    }

    // Wenn nicht, gibt es eben die klassische Hilfe
    puts(TMS(help_text,
        "Verfügbare Befehle:\n"

        "  <Pfad>         Startet das Programm an <Pfad> im Vordergrund\n"
        "  start <Pfad>   Startet das Programm an <Pfad> im Hintergrund\n"
        "  echo <Text>    Zeigt <Text> an\n"
        "\n"
        "  pwd            Zeigt den aktuellen Verzeichnisnamen an\n"
        "  ls             Zeigt den Verzeichnisinhalt an\n"
        "  stat           Zeigt Informationen zur einer Datei an\n"
        "  cd <Pfad>      Wechselt das Verzeichnis\n"
        "  mkdir <Pfad>   Erstellt das Verzeichnis\n"
        "\n"
        "  cp <Q> <Z>     Kopiert die Datei <Q> nach <Z>\n"
        "  ln <Q> <Z>     Erstellt einen auf <Q> verweisenden Link <Z>\n"
        "  symlink <Q><Z> Erstellt einen symbolischen Link <Z> auf <Q>\n"
        "  readlink <L>   Liest das Ziel des Symlinks <L> aus\n"
        "  rm <Pfad>      Löscht eine Datei\n"
        "\n"
        "  cat <Pfad>     Zeigt den Inhalt der Datei an\n"
        "  bincat <Pfad>  Zeigt den Inhalt der Datei als einzelne Bytes an\n"
        "  pipe <Pfad>    Ein-/Ausgabe auf einer Datei (z.B. TCP-Verbindung)\n"
        "  sync           Alle veränderten Blöcke im Cache zurückschreiben\n"
        "\n"
        "  date           Zeigt das Datum und die Uhrzeit an\n"
        "  free           Zeigt den Speicherverbrauch des Systems an\n"
        "  ps             Zeigt eine Liste mit allen Prozessen an\n"
        "  pstree         Zeigt die Prozesshierarchie als Baum an\n"
        "  kill <PID>     Beendet den Prozess <PID>\n"
        "  dbg_st <PID>   Zeigt einen Stack Backtrace des Prozesses <PID> an\n"

        "  set            Setzt Umgebungsvariablen oder zeigt sie an\n"
        "  read <S>       Liest einen Wert für die Umgebungsvariable <S> ein\n"
        "  source <S>     Führt das Skript <S> in der aktuellen Shell aus\n"
        "  sleep <S>      Wartet <S> Sekunden\n"

        "  exit           Beendet die Shell\n"
        "\n"
        "Diese Hilfe wird in den meisten Fällen nicht auf den Bildschirm"
        " passen. Um\n"
        "weiter oben stehende Befehle anzuzeigen, bitte Shift-BildAuf"
        " drücken.\n"
        "Shift-BildAb blättert wieder nach unten. Andere Tasten springen"
        " zurück an die\n"
        "aktuelle Position.\n"
        "\n"
    ));

    return 0;
}

/**
 *
 */
int shell_command_version(int argc, char* argv[])
{
    puts("tyndur Shell " TYNDUR_VERSION);
    return 0;
}

/**
 *
 */
int shell_command_exit(int argc, char* argv[])
{
    exit(0);
    return 0;
}

int shell_command_cd(int argc, char* argv[])
{
    char* path;
    DIR* dir;

    if (argc != 2) {
        goto usage;
    }

    path = io_get_absolute_path(argv[1]);
    if (path == NULL) {
        goto usage;
    }

    dir = opendir(path);
    if (dir == NULL) {
        printf(TMS(cd_error, "Wechseln in das Verzeichnis '%s' "
            "nicht möglich!\n"), path);
    } else {
        chdir(path);
        closedir(dir);
    }
    free(path);

    return 0;

usage:
    puts(TMS(cd_usage, "\nAufruf: cd <Verzeichnis>"));
    return -1;
}

int shell_command_source(int argc, char* argv[])
{
    const char* path;
    if (argc != 2) {
        printf(TMS(source_usage, "\nAufruf: source <Pfad>\n"));
        return -1;
    }

    path = argv[1];
    if (shell_script(path)) {
        return 0;
    } else {
        printf(TMS(source_error, "Ausführen des Skripts '%s' "
            "fehlgeschlagen.\n"), path);
        return -1;
    }
}

int shell_command_clear(int argc, char* argv[])
{
    printf("\033[1;1H\033[2J");

    return 0;
}

void set_display_usage(void);
void set_list_vars(void);

int shell_command_set(int argc, char* argv[])
{
    int overwrite = 1;

    while (argc > 1) {
        if (!strcmp(argv[1], "-d")) {
            overwrite = 0;
        } else if (!strcmp(argv[1], "-h")) {
            set_display_usage();
            return EXIT_SUCCESS;
        } else {
            break;
        }
        argc--;
        argv++;
    }

    switch (argc) {
        case 1:
            set_list_vars();
            break;

        case 2:
            unsetenv(argv[1]);
            break;

        case 3:
            setenv(argv[1], argv[2], overwrite);
            break;

        default:
            set_display_usage();
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void set_list_vars()
{
    int i, cnt;

    cnt = getenv_count();
    for (i = 0; i < cnt; i++) {
        printf("%s = %s\n", getenv_name_by_index(i), getenv_index(i));
    }
}

void set_display_usage()
{
    puts(TMS(set_usage, "\nAufruf: set [-d] [<Variable> [<Wert>]]\n\n"
        "Optionen:\n"
        "  -d: Defaultwert setzen. Wenn die Variable schon gesetzt ist, wird "
        "ihr Wert\n      nicht geändert.\n"));
}

