/*  
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
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

#include <stdint.h>
#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "string.h"
#include "init.h"
#include "stdlib.h"
#include <io.h>
#include <unistd.h>
#include <ctype.h>
#include <dir.h>

// Kindprozess eintragen
extern void wait_child_add(pid_t pid);

/**
 * Dem aktuellen Prozess beim Init-Prozess als Treiber anmelden.
 * @param name Der Treibername
 */
void init_service_register(char* name)
{
    const char* rpc_name = "SERV_REG";
    char* msg = malloc(strlen(rpc_name) + strlen(name) + 1);
    
    memcpy(msg, rpc_name, strlen(rpc_name));
    memcpy((char*)((uint32_t)msg + strlen(rpc_name)), name, strlen(name) + 1);
    
    
    send_message(INIT_PID, 512, 0, strlen(msg) + 1, msg);
}


/**
 * Die PID des Treibers beim Init-Prozess erfragen.
 * @param name Der Treibermame
 * @return Die Prozess ID des Treibers
 */
pid_t init_service_get(char* name)
{
    return rpc_get_dword(INIT_PID, "SERV_GET", strlen(name) + 1, name);
}

/**
 * Den Namen des Treibers beim Init-Prozess erfragen.
 * \param pid die pid des Prozesses
 * \return NULL, falls kein Treiber mit dieser pid registriert
 *         Name des Treibers andernfalls
 */
char *init_service_get_name(pid_t pid)
{
    return rpc_get_string(INIT_PID, "SERV_NAM", sizeof(pid_t), (void *)&pid);
}


/**
 * 
 * @param result Rueckgabewert des Tasks
 */
void init_process_exit(int result)
{
    const char* rpc_name = "SERV_EXI";
    char* msg = malloc(strlen(rpc_name) + 4 + 1);
    
    memcpy(msg, rpc_name, strlen(rpc_name));
    memcpy((char*)((uint32_t)msg + strlen(rpc_name)), &result, 4);
    msg[strlen(rpc_name) + 4] = 0;
    
    
    send_message(INIT_PID, 512, 0, strlen(msg), msg);

}

/**
 * Startet einen neuen Prozess
 *
 * @param cmd Dateiname des zu startenden Programms
 * @param ppb_shm_id ID des SHM, der den Process Parameter Block enthaelt
 */
pid_t __init_exec(const char* cmd, int ppb_shm_id)
{
    pid_t pid;

    // Vollstaendigen Pfad des Programmes finden
    size_t program_len = strcspn(cmd, " ");
    char program[program_len + 1];
    char* abs_path;

    strncpy(program, cmd, program_len);
    program[program_len] = 0;

    // Pruefen ob es sich um einen Pfad handelt
    abs_path = io_get_absolute_path(program);
    if (!abs_path || (access(abs_path, X_OK) != 0) || is_directory(abs_path)) {
        // Den absoluten Pfad freigeben
        free(abs_path);
        abs_path = NULL;

        // Es handelt sich nicht um eine Pfadangabe, dann wird jetzt PATH
        // durchsucht.
        const char* path = getenv("PATH");
        if (path == NULL) {
            return 0;
        }

        // Sonst wird jetzt eine Kopie davon angefertigt um sie auseinander zu
        // nehmen.
        char path_backup[strlen(path) + 1];
        char* dir;
        strcpy(path_backup, path);

        dir = strtok(path_backup, ";");
        while (dir != NULL) {
            // Verzeichnis + / + Programm + \0
            size_t dir_len = strlen(dir);
            char program_path[dir_len + 1 + program_len + 1];
            strcpy(program_path, dir);
            program_path[dir_len] = '/';
            strcpy(program_path + dir_len + 1, program);

            // Pruefen ob die Datei existiert
            if (access(program_path, X_OK) == 0) {
                // Wenn ja, wird ein Puffer alloziert und abs_path gesetzt
                abs_path = malloc(strlen(program_path) + 1);
                strcpy(abs_path, program_path);
                break;
            }

            dir = strtok(NULL, ";");
        }
    }

    // Wenn kein Programm gefunden wurde, wird abgebrochen
    if (abs_path == NULL) {
        return 0;
    }
    
    // Ansonsten wird jetzt der ganze Befehl in einen Puffer kopiert
    size_t abs_path_len = strlen(abs_path);
    size_t rpc_size = abs_path_len + sizeof(int) + 1;
    char rpc_data[rpc_size];
    strcpy(rpc_data, abs_path);
    *(int*)(rpc_data + abs_path_len + 1) = ppb_shm_id;
    free(abs_path);

    // TODO: Relative Pfade
    pid = rpc_get_dword(1, "LOADELF ", rpc_size, rpc_data);

    // Kindprozess eintragen, falls er erfolgreich gestartet wurde
    if (pid != 0) {
        wait_child_add(pid);
    }
    return pid;
}

/*
 * Zaehlt die Argumente in einer Kommandozeile. Mehrere aufeinanderfolgende
 * Leerzeichen werden als ein einziger Trenner aufgefasst.
 */
int cmdline_get_argc(const char* args)
{
    bool space = false;
    int argc = 0;
    int pos;

    for (pos = 0; pos < strlen(args); pos++) {
        if (isspace(args[pos]) && !space) {
            argc++;
        }

        space = isspace(args[pos]);
    }

    if (!space) {
        argc++;
    }

    return argc;
}

/*
 * Trennt einen Argumentstring an den Leerzeichen auf und befüllt das
 * argv-Array mit Pointern auf die einzelnen Teilstrings.
 *
 * Es wird keine Kopie von args angelegt, d.h. der ursprüngliche String wird
 * modifiziert und darf nicht freigegeben werden, solange argv noch in
 * Benutzung ist!
 */
void cmdline_copy_argv(char* args, const char** argv, size_t argc)
{
    int pos;
    argv[0] = strtok(args, " ");

    for(pos = 1; pos < argc; pos++) {
        argv[pos] = strtok(NULL, " ");
    }

    argv[argc] = NULL;
}

pid_t init_execute(const char* args)
{
    int argc;
    char* s;
    int ret;

    /* Zunaechst die Argumente zaehlen. */
    argc = cmdline_get_argc(args);

    /* Jetzt argv anlegen und befuellen */
    const char* argv[argc + 1];

    s = strdup(args);
    cmdline_copy_argv(s, argv, argc);

    /* Und den Prozess starten */
    ret = init_execv(args, argv);
    free(s);

    return ret;
}

int init_dev_register(struct init_dev_desc* dev, size_t size)
{
    return rpc_get_int(1, "DEV_REG ", size, (char*) dev);
}

int init_dev_list(struct init_dev_desc** devs)
{
    int res;
    response_t* response = rpc_get_response(1, "DEV_LIST", 0, NULL);

    if (response == 0) {
        return -1;
    }

    *devs = response->data;
    res = response->data_length;
    free(response);

    return res;
}
