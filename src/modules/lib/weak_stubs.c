/*
 * Copyright (c) 2017 The tyndur Project. All rights reserved.
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
#include <stdbool.h>
#include <types.h>
#include <errno.h>

/* Diese Funktionen und Variablen sind dazu gedacht, vom Programm überladen
 * zu werden. Wir definieren hier leere Stubs, um in Programmen, die die
 * entsprechenden Funktionen nicht benutzen, Undefined References zu
 * vermeiden. */

asm(".global __start_tmslang\n"
    "__start_tmslang:\n"
    ".global __stop_tmslang\n"
    "__stop_tmslang:\n");

__attribute__((weak))
vaddr_t loader_allocate_mem(size_t size)
{
    abort();
    return NULL;
}

__attribute__((weak))
bool loader_assign_mem(pid_t process, vaddr_t dest_address,
    vaddr_t src_address, size_t size)
{
    abort();
    return false;
}

__attribute__((weak))
int loader_get_library(const char* name, void** image, size_t* size)
{
    abort();
    return -ENOSYS;
}

__attribute__((weak))
bool loader_create_thread(pid_t process, vaddr_t address)
{
    abort();
    return false;
}
