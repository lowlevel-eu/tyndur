/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
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

#include <stdint.h>
#include "syscall.h"

dma_mem_ptr_t mem_dma_allocate(uint32_t size, uint32_t flags)
{
    dma_mem_ptr_t ptr;

    asm(
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0xc, %%esp;"
    : "=a" (ptr.virt)
    : "i" (SYSCALL_MEM_ALLOCATE), "r" (size), "r" (flags), "r" (&ptr.phys)
    : "memory");

    return ptr;
}

void* mem_allocate(uint32_t size, uint32_t flags)
{
    dma_mem_ptr_t ptr = mem_dma_allocate(size, flags);
    return ptr.virt;
}

void *mem_allocate_physical(uint32_t size, uint32_t position, uint32_t flags)
{
    dma_mem_ptr_t ptr;

    asm(
            "pushl %4;"
            "pushl %3;"
            "pushl %2;"
            "mov %1, %%eax;"
            "int $0x30;"
            "add $0xC, %%esp;"
    : "=a" (ptr.virt) : "i" (SYSCALL_MEM_ALLOCATE_PHYSICAL), "r" (size), "r" (position), "r" (flags));

    return ptr.virt;
}

bool mem_free(void* address, uint32_t size)
{
    uint32_t result;
    asm(
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x8, %%esp;"
    : "=a" (result) : "i" (SYSCALL_MEM_FREE), "r" (address), "r" (size));

    return result;
}

void mem_free_physical(void* address, uint32_t size)
{
    asm(
            "pushl %2;"
            "pushl %1;"
            "mov %0, %%eax;"
            "int $0x30;"
            "add $0x8, %%esp;"
    : : "i" (SYSCALL_MEM_FREE_PHYSICAL), "r" (address), "r" (size));
}
