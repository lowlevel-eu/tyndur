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
#include <syscall.h>
#include <rpc.h>
#include <init.h>
#include <string.h>
#include <stdio.h>
#include <lostio.h>

//Sollte anders geloest werden
extern int main(int argc, char* argv[]);
extern void init_sync_messages(void);
extern void init_envvars(void* ppb, size_t ppb_size);
extern void init_waitpid(void);
extern void stdio_init(void);

extern void (*__ctor_start__)(void);
extern void (*__ctor_end__)(void);

static void call_constructors(void)
{
    void (**f)(void);

    for (f = &__ctor_start__; f < &__ctor_end__; f++) {
        /* Dummy-Eintrag aus crtbegin.o ueberspringen */
        if (*f && ((uintptr_t) *f != 0xffffffff)) {
            (*f)();
        }
    }
}

asm (
    ".global _start\n"
    ".weak _start\n"
    "_start:\n"
    "   push %ecx\n"
    "   push %ebx\n"
    "   push %eax\n"
    "   call c_start\n"
    "   ud2\n"
);

static void __attribute__((used))
    c_start(uint32_t ppb_shm_id, void* ppb, size_t ppb_size)
{
    char* args = NULL;
    int argc;

    init_memory_manager();
    init_messaging();
    init_sync_messages();

    init_envvars(ppb, ppb_size);
    init_waitpid();
#ifndef _NO_STDIO_
    if (ppb_shm_id) {
        lio_stream_t streams[3];
        int ret = ppb_get_streams(streams, ppb, ppb_size, 1, 3);
        if (ret == 3) {
            stdin = streams[0] ? __create_file_from_lio_stream(streams[0]) : NULL;
            stdout = streams[1] ? __create_file_from_lio_stream(streams[1]) : NULL;
            stderr = streams[2] ? __create_file_from_lio_stream(streams[2]) : NULL;
        }
    }
    stdio_init();
#endif

    //Kommandozeilenargumente Parsen ----
    //TODO: Das ist alles noch sehr primitiv
    //Erst den String vom Kernel holen
    if (ppb_shm_id) {
        argc = ppb_get_argc(ppb, ppb_size);
        if (argc < 0) {
            printf("Ungueltiger PPB!\n");
            abort();
        }
    } else {
        args = get_cmdline();
        argc = cmdline_get_argc(args);
    }

    char* argv[argc + 1];

    if (ppb_shm_id) {
        ppb_copy_argv(ppb, ppb_size, argv, argc);
    } else {
        cmdline_copy_argv(args, (const char**) argv, argc);
    }

    call_constructors();
    int result = main(argc, argv);

    // Falls ein Buffer auf stdout eingerichtet wurde, wird der geflusht und
    // freigegeben.
    // FIXME: Alle Buffer flushen!!
#ifndef _NO_STDIO_
    fflush(stdout);
#endif

    exit(result);

    //Sollte niemals passieren
    while(1);
}


