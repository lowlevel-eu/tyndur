/*
 * Copyright © 2012 The týndur Project. All rights reserved.
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

#include "gdt.h"

#define SEGMENT_DATA    0x0000120000000000ULL
#define SEGMENT_CODE    0x0000180000000000ULL
#define SEGMENT_TSS     0x0000090000000000ULL
#define SEGMENT_USER    0x0000600000000000ULL
#define SEGMENT_PRESENT 0x0000800000000000ULL
#define SEGMENT_LM      0x0020000000000000ULL
#define SEGMENT_32BIT   0x0040000000000000ULL
#define SEGMENT_GRAN    0x0080000000000000ULL
#define SEGMENT_BASE(x)   (((x & 0xffffULL) << 16) | ((x & 0xff0000ULL) << 16) | ((x & 0xff000000ULL) << 32))
#define SEGMENT_BASE64(x) ((x >> 32) & 0xffffffffULL)
#define SEGMENT_SIZE(x)   ((x & 0xffffULL) | ((x & 0xf0000ULL) << 32))

struct {
    uint16_t limit;
    uintptr_t base;
} gdtr;

/// Global Descritpor Table
uint64_t gdt[9] = {
    // NULL Deskriptor
    0,

    // Kernel Code-Segment 32Bit
    SEGMENT_BASE(0LL) | SEGMENT_SIZE(0xfffffLL) | SEGMENT_GRAN | SEGMENT_CODE |
            SEGMENT_32BIT | SEGMENT_PRESENT,

    // Kernel Data-Segment
    SEGMENT_BASE(0LL) | SEGMENT_SIZE(0xfffffLL) | SEGMENT_GRAN | SEGMENT_DATA |
            SEGMENT_32BIT | SEGMENT_PRESENT,

    // Kernel Code-Segment 64Bit
    SEGMENT_CODE | SEGMENT_LM | SEGMENT_PRESENT,

    // User Code-Segment 32Bit
    SEGMENT_BASE(0LL) | SEGMENT_SIZE(0xfffffLL) | SEGMENT_GRAN | SEGMENT_CODE |
            SEGMENT_32BIT | SEGMENT_USER | SEGMENT_PRESENT,

    // User Data-Segment
    SEGMENT_BASE(0LL) | SEGMENT_SIZE(0xfffffLL) | SEGMENT_GRAN | SEGMENT_DATA |
            SEGMENT_32BIT | SEGMENT_USER | SEGMENT_PRESENT,

    // User Code-Segment 64Bit
    SEGMENT_CODE | SEGMENT_LM | SEGMENT_USER | SEGMENT_PRESENT,

    // Task State - Segment
    SEGMENT_BASE((uintptr_t) 0 /*TODO*/) |
            SEGMENT_SIZE(0x67) | SEGMENT_TSS | SEGMENT_PRESENT,
    SEGMENT_BASE64((uintptr_t) 0 /*TODO*/)
};

void gdt_init()
{
    gdtr.base = (uintptr_t) gdt;
    gdtr.limit = sizeof(gdt) - 1;
}

void gdt_init_local()
{
    asm volatile ("lgdt %0\n" : : "m"(gdtr));
}
