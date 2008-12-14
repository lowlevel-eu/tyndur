/*
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
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
 *	    This product includes software developed by the tyndur Project
 *	    and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE tyndur PROJECT AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE tyndur PROJECT OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <types.h>

#include "cpu.h"
#include "lock.h"

#define GDT_SIZE 255

#define GDT_CODESEG 0x0A
#define GDT_DATASEG 0x02
#define GDT_TSS 0x09
#define GDT_PRESENT 0x80
#define GDT_SEGMENT 0x10

typedef struct {
  word size;
  word base;
  byte base2;
  byte access;
  byte size2;
  byte base3;
} segment_descriptor;

/// Die GDT
segment_descriptor gdt[GDT_SIZE];

/// Letzter Eintrag in der GDT
word next_entry_index = 0;

/// Lock fuer die GDT
lock_t gdt_lock = 0;

void gdt_init_local();
static void gdt_set_descriptor(int segment, dword size, dword base, 
    byte access, int dpl);
static void gdt_set_descriptor_byte_granularity(int segment, dword size, 
    dword base, byte access, int dpl);

/**
 * Legt eine GDT an und initialisiert sie mit mit jeweils einem Code- und
 * Datendeskriptor für Ring 0 und Ring 3.
 * Anschliessend wird sie geladen
 */
void gdt_init()
{
    // Ring-0-Code- und Datendeskriptoren eintragen
    gdt_set_descriptor(1, 0x000FFFFF, 0x00000000, GDT_SEGMENT | GDT_PRESENT |
        GDT_CODESEG, 0);
    gdt_set_descriptor(2, 0x000FFFFF, 0x00000000, GDT_SEGMENT | GDT_PRESENT |
        GDT_DATASEG, 0);

    // Ring-3-Code- und Datendeskriptoren eintragen
    gdt_set_descriptor(3, 0x000FFFFF, 0x00000000, GDT_SEGMENT | GDT_PRESENT |
        GDT_CODESEG, 3);
    gdt_set_descriptor(4, 0x000FFFFF, 0x00000000, GDT_SEGMENT | GDT_PRESENT |
        GDT_DATASEG, 3);
    
    // Ab diesem Index werden die TSS eingetragen
    next_entry_index = 5;
    
    // Dafuer sorgen, dass der Lock sicher nicht gesperrt ist
    unlock(&gdt_lock);

    gdt_init_local();
}

/**
 *
 */
void gdt_init_local()
{
    lock(&gdt_lock);
    // Deskriptor fuer das Task state segment anlegen
    gdt_set_descriptor_byte_granularity(next_entry_index++, sizeof(cpu_tss_t) -
    1, (dword) &(cpu_get_current()->tss), GDT_PRESENT | GDT_TSS, 3);

    // GDTR laden
    struct {
        word size;
        dword base;
    }  __attribute__((packed)) gdt_ptr = {
        .size  = GDT_SIZE*8 - 1,
        .base  = (dword)gdt,
    };

    __asm__("lgdtl %0\n\t"
        "ljmpl $0x08, $1f\n\t"
        "1:\n\t"
        "mov $0x10, %%eax\n\t"
        "mov %%eax, %%ds\n\t"
        "mov %%eax, %%es\n\t"
        "mov %%eax, %%fs\n\t"
        "mov %%eax, %%gs\n\t"
        "mov %%eax, %%ss\n\t" : : "m" (gdt_ptr) : "eax");

    __asm__("ltr %%ax\n\t" : : "a" ((next_entry_index - 1) << 3));

    unlock(&gdt_lock);
}

/**
 * Setzt einen Deskriptor in der GDT.
 *
 * @param segment Nummer des Deskriptors
 * @param size Größe des Segments in Pages
 * @param base Basisadresse des Segments
 * @param access Access-Byte des Deskriptors
 * @param dpl Descriptor Privilege Level
 */
static void gdt_set_descriptor(int segment, dword size, dword base, 
    byte access, int dpl)
{
    gdt[segment].size   = size & 0xFFFF;
    gdt[segment].size2  = ((size >> 16) & 0x0F) | 0xC0;
    gdt[segment].base   = base & 0xFFFF;
    gdt[segment].base2  = (base >> 16) & 0xFF;
    gdt[segment].base3  = ((base >> 24) & 0xFF);
    gdt[segment].access = access | ((dpl & 3) << 5);
}

/**
 * Setzt einen Deskriptor in der GDT, wobei die Größe als Byteangabe
 * interpretiert wird.
 *
 * @param size Größe des Segments in Bytes
 * @param base Basisadresse des Segments
 * @param access Access-Byte des Deskriptors
 * @param dpl Descriptor Privilege Level
 */
static void gdt_set_descriptor_byte_granularity(int segment, dword size,
    dword base, byte access, int dpl)
{
    gdt_set_descriptor(segment, size, base, access, dpl);
    gdt[segment].size2  = ((size >> 16) & 0x0F) | 0x40;
}
