/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
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

#include <stdlib.h>
#include "syscall.h"
#include "rpc.h"
#include <string.h>
#include <stdio.h>

//Sollte anders geloest werden
extern int main(int argc, char* argv[]);
extern void init_sync_messages(void);
extern void init_envvars(void);
extern void init_waitpid(void);
extern void stdio_init(void);

void __attribute__((weak)) _start(void)
{
    //v();
    init_memory_manager();
    init_messaging();
    init_sync_messages();

    init_envvars();
    init_waitpid();
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
            // mehrere leerzeichen direkt hintereinander nur einmal z�hlen
            if(f == 1)
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

    char* argv[argc + 1];
    argv[0] = strtok(args, " ");
    for(pos = 1; pos < argc; pos++)
    {
        argv[pos] = strtok(NULL, " ");
    }
    argv[argc] = NULL;

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


