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

#include "syscall.h"


void init_child_page (pid_t pid, void* dest, void* src, size_t size)
{
    asm(
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x10, %%esp;"
    : : "i" (SYSCALL_PM_INIT_PAGE), "r"(pid), "r" (dest), "r" (src), "r" (size));
}

void init_child_page_copy (pid_t pid, void* dest, void* src, size_t size)
{
    asm(
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x10, %%esp;"
    : : "i" (SYSCALL_PM_INIT_PAGE_COPY), "r"(pid), "r" (dest), "r" (src), "r" (size));
}

int init_child_ppb(pid_t pid, int shm)
{
    int ret;

    asm(
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x08, %%esp;"
    : "=a" (ret)
    : "i" (SYSCALL_PM_INIT_PROC_PARAM_BLOCK), "r"(pid), "r" (shm));

    return ret;
}
