/*
 * Copyright (c) 2006 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
 *	    This product includes software developed by the LOST Project
 *	    and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE LOST PROJECT AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE LOST PROJECT OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Vorl‰ufige Funktionen zur Verwaltung der GDT
 *
 * Aenderungen:
 *
 * 2006-04-09: taljeth
 *   + Erste Version erstellta
 */

#include "types.h"
#include "gdt.h"

typedef struct {
  word size;
  word base;
  byte base2;
  byte access;
  byte size2;
  byte base3;
} segment_descriptor;

segment_descriptor *gdt;

/**
 * Legt eine GDT an der Adresse GDT_BASE an und initialisiert sie mit
 * Null-Deskriptoren sowie je einem Code- und Datensegment.
 * Anschlieﬂend wird sie geladen.
 */
void init_gdt()
{
  int i;

  // GDT anlegen und mit Null-Deskriptoren initialisieren
  gdt = (segment_descriptor*) GDT_BASE;
  for (i = 0; i < GDT_SIZE; i++) {
    gdt_set_descriptor(i, 0, 0, 0);
  }

  // Ein Code- und ein Datensegment eintragen
  gdt_set_descriptor(1, 0x000FFFFF, 0x00000000, SEGMENT | PRESENT | CODESEG);
  gdt_set_descriptor(2, 0x000FFFFF, 0x00000000, SEGMENT | PRESENT | DATASEG);

  // GDTR laden
  struct {
    word size;
    word base;
    word base2;
  } gdt_ptr = {
    .size  = GDT_SIZE*8,
    .base  = GDT_BASE & 0xFFFF,
    .base2 = GDT_BASE >> 16
  };

  asm("lgdtl %0\n\t"
    "ljmpl $0x08, $1f\n\t"
    "1:\n\t"
    "mov $0x10, %%eax\n\t"
    "mov %%eax, %%ds\n\t"
    "mov %%eax, %%es\n\t"
    "mov %%eax, %%fs\n\t"
    "mov %%eax, %%gs\n\t"
    "mov %%eax, %%ss\n\t" : : "g" (gdt_ptr));
}

/**
 * Setzt einen Deskriptor in der GDT.
 *
 * \param segment Nummer des Deskriptors
 * \param size Grˆﬂe des Segments
 * \param base Basisadresse des Segments
 * \param access Access-Byte des Deskriptors
 */
void gdt_set_descriptor(int segment, dword size, dword base, byte access)
{
  gdt[segment].size   = size & 0xFFFF;
  gdt[segment].size2  = ((size >> 16) & 0x0F) | 0xC0;
  gdt[segment].base   = base & 0xFFFF;
  gdt[segment].base2  = (base >> 16) & 0xFF;
  gdt[segment].base3  = ((base >> 24) & 0xFF);
  gdt[segment].access = access;
}
