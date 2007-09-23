/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
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
#ifndef _INIT_H_
#define _INIT_H_

#include <types.h>
#include <stdio.h>

#define INIT_PID 1


#ifndef MODULE_INIT

void init_service_register(char* name);
pid_t init_service_get(char* name);

void init_process_exit(int result);
pid_t init_execute(const char* cmd);


#endif //ifndef _INIT_H_


#ifdef _USE_START_
#undef _USE_START_
#include <stdlib.h>
#include "syscall.h"
#include "rpc.h"
#include <string.h>
//Sollte anders geloest werden
extern int main(int argc, char* argv[]);
extern void init_sync_messages();
extern void init_envvars();
extern void stdio_init();
void _start() {
    //v();
    init_memory_manager();
    init_messaging();
    init_sync_messages();

    init_envvars();
#ifndef _NO_STDIO_
    stdio_init();
  #ifndef _NO_STDIO_BUFFER_
    // Buffer fuer stdout allozieren
    size_t stdout_buffer_size = 256;
    void* stdout_buffer = malloc(stdout_buffer_size);

    // Buffer einrichten
    setvbuf(stdout, stdout_buffer, _IOLBF, stdout_buffer_size);
  #endif  
#endif
    
    
    //Kommandozeilenargumente Parsen ---- 
    //TODO: Das ist alles noch sehr primitiv
    //Erst den String vom Kernel holen
    char* args = get_cmdline();
    int argc, pos, f;
    argc = 0;
    f = 0;
    //Jetzt werden die Argumente gezaehlt.
    for (pos = 0; pos < strlen(args); pos++) {
        if (args[pos] == ' ') {
            if(f == 1) // mehrere leerzeichen direkt hintereinander nur einmal zählen
            {
                argc++;
            }
            f = 0;
        }
        else
        {
            f = 1;
        }
    }
    if(f == 1)
    {
        argc++;
    }
    
    char* argv[argc];
    argv[0] = strtok(args, " ");
    for(pos = 1; pos < argc; pos++)
    {
        argv[pos] = strtok(NULL, " ");
    }

    

    int result = main(argc, argv);
    
    // Falls ein Buffer auf stdout eingerichtet wurde, wird der geflusht und
    // freigegeben.
    // FIXME: Alle Buffer flushen!!
#ifndef _NO_STDIO_
  #ifndef _NO_STDIO_BUFFER_
    fflush(stdout);
    free(stdout_buffer);
  #endif
#endif

    exit(result);

    //Sollte niemals passieren
    while(1);
}


#else
    struct service_s
    {
        pid_t   pid;
        char*   name;
    };
    
    struct service_s* get_service_by_name(const char* name);
    struct service_s* get_service_by_pid(pid_t pid);

#endif

#endif

