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

/*
 * Vorläufige Funktionen zur Verwaltung der GDT
 *
 * Aenderungen:
 *
 * 2006-04-09: taljeth
 *   + Erste Version erstellta
 */

#include <stdint.h>

#include "gdt.h"
#include "tss.h"

typedef struct {
  uint16_t size;
  uint16_t base;
  uint8_t base2;
  uint8_t access;
  uint8_t size2;
  uint8_t base3;
} segment_descriptor;

segment_descriptor gdt[GDT_SIZE];

/**
 * Legt eine GDT an und initialisiert sie mit mit jeweils einem Code- und
 * Datendeskriptor für Ring 0 und Ring 3 und einem Deskriptor für die TSS.
 * Anschließend wird sie geladen.
 */
void init_gdt()
{
  // Ring-0-Code- und Datenselektoren eintragen
  gdt_set_descriptor(1, 0x000FFFFF, 0x00000000, GDT_SEGMENT | GDT_PRESENT | GDT_CODESEG, 0);
  gdt_set_descriptor(2, 0x000FFFFF, 0x00000000, GDT_SEGMENT | GDT_PRESENT | GDT_DATASEG, 0);

  // Ring-3-Code- und Datenselektoren eintragen
  gdt_set_descriptor(3, 0x000FFFFF, 0x00000000, GDT_SEGMENT | GDT_PRESENT | GDT_CODESEG, 3);
  gdt_set_descriptor(4, 0x000FFFFF, 0x00000000, GDT_SEGMENT | GDT_PRESENT | GDT_DATASEG, 3);

  // Task irgendwas
  gdt_set_descriptor_byte_granularity(5, sizeof(tss) - 1,
    (uint32_t)&tss, GDT_PRESENT | GDT_TSS, 3);

  // GDTR laden
  struct {
    uint16_t size;
    uint32_t base;
  }  __attribute__((packed)) gdt_ptr = {
    .size  = GDT_SIZE*8 - 1,
    .base  = (uint32_t)gdt,
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

  __asm__("ltr %%ax\n\t" : : "a" (TSS_SEL));
}

/**
 * Setzt einen Deskriptor in der GDT.
 *
 * \param segment Nummer des Deskriptors
 * \param size Größe des Segments in Pages
 * \param base Basisadresse des Segments
 * \param access Access-Byte des Deskriptors
 * \param dpl Descriptor Privilege Level
 */
void gdt_set_descriptor(int segment, uint32_t size, uint32_t base, uint8_t access,
    int dpl)
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
 * \param segment Nummer des Deskriptors
 * \param size Größe des Segments in Bytes
 * \param base Basisadresse des Segments
 * \param access Access-Byte des Deskriptors
 * \param dpl Descriptor Privilege Level
 */
void gdt_set_descriptor_byte_granularity(int segment, uint32_t size,
    uint32_t base, uint8_t access, int dpl)
{
  gdt_set_descriptor(segment, size, base, access, dpl);
  gdt[segment].size2  = ((size >> 16) & 0x0F) | 0x40;
}
