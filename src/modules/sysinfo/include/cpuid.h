/*  
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Alexander Siol.
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
#ifndef _CPUID_H_
#define _CPUID_H_

#include <stdint.h>
#include <stdbool.h>
#include "io.h"
#include "syscall.h"

// Strukturen

/**
 * Struktur fuer die Infos aus CPUID, Level 0
 * Enthaelt max_level (maximaler level in cpuid) und die vendorID (Hersteller)
 */
typedef struct {
    uint32_t max_level;
    char  vendorID[13];
} cpuid_0;

/**
 * Struktur fuer die Infos aus CPUID, Level 1
 * Enthaelt Infos ueber den APIC (ebx) sowie die Standardflags (edx, ecx)
 */
typedef struct {
    uint32_t eax;
    union {
        uint32_t ebx;
        struct  {
            uint8_t apic_id;
            uint8_t cpu_count;
            uint8_t clflush;
            uint8_t brand_id;
        };
    };
    uint32_t ecx;
    uint32_t edx;
} cpuid_1;

/**
 * Struktur fuer die Infos aus CPUID, Extended Level 0
 * Enthaelt max_level und vendorID, wie Level 0
 */
typedef struct {
    uint32_t max_level;
    char vendorID[13];
} cpuid_e0;

/**
 * Struktur fuer die Infos aus CPUID, Extended Level 1
 * Enthaelt die erweiterten Flags in edx und ecx
 */
typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} cpuid_e1;

/**
 * Struktur fuer die Infos aus CPUID, Extended Level 2-4
 * Enthaelt den Processor Brand String, eine Markenangabe des Prozessors.
 */
typedef struct {
    uint32_t eax;
    char proc_brand_string[50];
} cpuid_e2_e4;

// Funktionen

/**
 * Prueft ob CPUID vorhanden ist.
 *
 * @return true wenn cpuid funktioniert, sonst false
 */
bool has_cpuid(void);

/**
 * Macht einen Aufruf an CPUID. Achtung, die Parameter werden auch zur Rueckgabe
 * verwendet!
 *
 * @param eax Pointer auf Werte fuer eax. Eingangswert: Level von CPUID
 * @param ebx Pointer auf Rueckgabewert fuer ebx
 * @param ecx Pointer auf Rueckgabewert fuer ecx
 * @param edx Pointer auf Rueckgabewert fuer edx
 */
void cpuid(uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx);

/**
 * Fragt CPUID Level 0 ab
 *
 * @see cpuid_0
 * @return cpuid_0, enthaelt alle Infos von Level 0
 */
cpuid_0 cpuid_level_0(void);

/**
 * Fragt CPUID Level 1 ab
 *
 * @see cpuid_1
 * @return cpuid_1, enthaelt alle Infos von Level 1 (APIC Infos, Flags)
 */
cpuid_1 cpuid_level_1(void);

/**
 * Fragt CPUID Extended Level 0 ab
 *
 * @see cpuid_e0
 * @return cpuid_e0, enthaelt alle Infos von Extended Level 0 (aehnlich Level 0)
 */
cpuid_e0 cpuid_level_e0(void);

/**
 * Fragt CPUID Extended Level 1 ab
 *
 * @see cpuid_e1
 * @return cpuid_e1, enthaelt alle Infos von Extended Level 1 (Flags)
 */
cpuid_e1 cpuid_level_e1(void);

/**
 * Fragt CPUID Extended Level 2 bis 4 ab
 *
 * @see cpuid_e2_e4
 * @return cpuid_e2_e4, alle Infos dieser Level ( = den Processor Brand String)
 */
cpuid_e2_e4 cpuid_level_e2_e4(void);

// Flags Level 1 - EDX - Alle CPUs

#define CPUID_FLAG_FPU      ( 1 << 0 )
#define CPUID_FLAG_VME      ( 1 << 1 )
#define CPUID_FLAG_DE       ( 1 << 2 )
#define CPUID_FLAG_PSE      ( 1 << 3 )
#define CPUID_FLAG_TSC      ( 1 << 4 )
#define CPUID_FLAG_MSR      ( 1 << 5 )
#define CPUID_FLAG_PAE      ( 1 << 6 )
#define CPUID_FLAG_MCE      ( 1 << 7 )
#define CPUID_FLAG_CX8      ( 1 << 8 )
#define CPUID_FLAG_APIC     ( 1 << 9 )
#define CPUID_FLAG_SEP      ( 1 << 11 )
#define CPUID_FLAG_MTRR     ( 1 << 12 )
#define CPUID_FLAG_PGE      ( 1 << 13 )
#define CPUID_FLAG_MCA      ( 1 << 14 )
#define CPUID_FLAG_CMOV     ( 1 << 15 )
#define CPUID_FLAG_PAT      ( 1 << 16 )
#define CPUID_FLAG_PSE36    ( 1 << 17 )
#define CPUID_FLAG_PSN      ( 1 << 18 )
#define CPUID_FLAG_CLFL     ( 1 << 19 )
#define CPUID_FLAG_DTES     ( 1 << 21 )
#define CPUID_FLAG_ACPI     ( 1 << 22 )
#define CPUID_FLAG_MMX      ( 1 << 23 )
#define CPUID_FLAG_FXSR     ( 1 << 24 )
#define CPUID_FLAG_SSE      ( 1 << 25 )
#define CPUID_FLAG_SSE2     ( 1 << 26 )
#define CPUID_FLAG_SS       ( 1 << 27 )
#define CPUID_FLAG_HTT      ( 1 << 28 )
#define CPUID_FLAG_TM1      ( 1 << 29 )
#define CPUID_FLAG_IA64     ( 1 << 30 )
#define CPUID_FLAG_PBE      ( 1 << 31 )

// Flags Level 1 - ECX - Alle CPUs

#define CPUID_FLAG_SSE3     ( 1 << 0 )
#define CPUID_FLAG_MON      ( 1 << 3 )
#define CPUID_FLAG_DSCPL    ( 1 << 4 )
#define CPUID_FLAG_VMX      ( 1 << 5 )
#define CPUID_FLAG_SMX      ( 1 << 6 )
#define CPUID_FLAG_EST      ( 1 << 7 )
#define CPUID_FLAG_TM2      ( 1 << 8 )
#define CPUID_FLAG_SSSE3    ( 1 << 9 )
#define CPUID_FLAG_CID      ( 1 << 10 )
#define CPUID_FLAG_CX16     ( 1 << 13 )
#define CPUID_FLAG_ETPRD    ( 1 << 14 )
#define CPUID_FLAG_PDCM     ( 1 << 15 )
#define CPUID_FLAG_DCA      ( 1 << 18 )
#define CPUID_FLAG_SSE41    ( 1 << 19 )
#define CPUID_FLAG_SSE42    ( 1 << 20 )
#define CPUID_FLAG_X2APIC   ( 1 << 21 )
#define CPUID_FLAG_POPCNT   ( 1 << 23 )

// Flags Extended Level 1 - ECX - AMD (nicht angegebene reserviert)
#define CPUID_FLAG_LAHFSAHF ( 1 << 0 )
#define CPUID_FLAG_CMPLEGACY    ( 1 << 1 )
#define CPUID_FLAG_SVM      ( 1 << 2 )
#define CPUID_FLAG_EXTAPIC  ( 1 << 3 )
#define CPUID_FLAG_ALTMOVCR8    ( 1 << 4 )
#define CPUID_FLAG_ABM      ( 1 << 5 )
#define CPUID_FLAG_SSE4A    ( 1 << 6 )
#define CPUID_FLAG_MISALIGNSSE  ( 1 << 7 )
#define CPUID_FLAG_3DNOWPREFETCH    ( 1 << 8 )
#define CPUID_FLAG_OSVW     ( 1 << 9 )
#define CPUID_FLAG_SKINIT   ( 1 << 12 )
#define CPUID_FLAG_WDT      ( 1 << 13 )

// Flags Extended Level 1 - EDX - AMD
// (nicht angegebene reserviert oder identisch mit Level 1 EDX)
#define CPUID_FLAG_NX       ( 1 << 20 )
#define CPUID_FLAG_MMXEXT   ( 1 << 22 )
#define CPUID_FLAG_FFXSR    ( 1 << 25 )
#define CPUID_FLAG_PAGE1GB  ( 1 << 26 )
#define CPUID_FLAG_RDTSCP   ( 1 << 27 )
#define CPUID_FLAG_LM       ( 1 << 29 )
#define CPUID_FLAG_3DNOWEXT ( 1 << 30 )
#define CPUID_FLAG_3DNOW    ( 1 << 31 )

// Strings (zur Verwendung in der Ausgabe)

extern char flags_01_edx[32][6];
extern char flags_01_ecx[32][6];
extern char flags_e01_ecx_amd[32][20];
extern char flags_e01_edx_amd[32][20];

#endif
