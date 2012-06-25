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

#include <stdint.h>
#include <stdbool.h>
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "cpuid.h"

#define DEBUG_MSG(s) printf("[ CPUID ] debug: %s() '%s'\n", __FUNCTION__, s)

// Hier werden die Strings fuer die diversen Flags definiert

// Basic Flags, auf allen Prozessoren identisch
char flags_01_edx[32][6] = {
    "fpu", "vme", "de", "pse", "tsc", "msr", "pae", "mce", "cx8", "", "apic",
    "sep", "mtrr", "pge", "mca", "cmov", "pat", "pse36", "psn", "clfl", "",
    "dtes", "acpi", "mmx", "fxsr", "sse", "sse2", "ss", "htt", "tm1", "ia-64",
    "pbe"
};
char flags_01_ecx[32][6] = {
    "sse3", "", "", "mon", "dscpl", "vmx", "smx", "est", "tm2", "ssse3", "cid",
    "", "", "cx16", "etprd", "pdcm", "", "", "dca", "sse4.1", "sse4.2",
    "x2apic", "", "popcnt", "", "", "",  "", "", "", "", ""
};

// Extended Flags fuer AMD-Prozessoren - unterscheiden sich von denen von Intel
char flags_e01_ecx_amd[32][20] = {
    "lahfsahf", "cmplegacy", "svm", "extapic", "altmovcr8", "abm", "sse4a",
    "misalignsse", "3dnowprefetch", "osvw", "", "", "skinit", "wdt"
};
char flags_e01_edx_amd[32][20] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "nx", "", "mmxext", "", "", "ffxsr", "page1gb", "rdtscp", "lm",
    "3dnowext", "3dnow"
};

/**
 * Prueft ob CPUID vorhanden ist.
 *
 * @return true wenn cpuid funktioniert, sonst false
 */
bool has_cpuid()
{
    int result;
    asm (
        "pushfl;"
        "popl %%eax;"
        "movl %%ebx, %%eax;"
        "xorl $0x200000, %%eax;"
        "pushl %%eax;"
        "popfl;"
        "pushfl;"
        "pop %%eax;"
        "xorl %%eax, %%ebx;"
        : "=a"(result)
    );
    return result;
}

/**
 * Macht einen Aufruf an CPUID. Achtung, die Parameter werden auch zur Rueckgabe
 * verwendet!
 *
 * @param eax Pointer auf Werte fuer eax. Eingangswert: Level von CPUID
 * @param ebx Pointer auf Rueckgabewert fuer ebx
 * @param ecx Pointer auf Rueckgabewert fuer ecx
 * @param edx Pointer auf Rueckgabewert fuer edx
 */
void cpuid(uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx)
{
    asm volatile("cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(*eax), "b"(*ebx), "c"(*ecx), "d"(*edx)
    );
}

/**
 * Fragt CPUID Level 0 ab
 *
 * @see cpuid_0
 * @return cpuid_0, enthaelt alle Infos von Level 0
 */
cpuid_0 cpuid_level_0()
{
    cpuid_0 result;
    result.max_level = 0;
    result.vendorID[12] = '\0';
    cpuid(&result.max_level,
        (uint32_t*)(&result.vendorID[0]),
        (uint32_t*)(&result.vendorID[8]),
        (uint32_t*)(&result.vendorID[4]));
    return result;
}

/**
 * Fragt CPUID Level 1 ab
 *
 * @see cpuid_1
 * @return cpuid_1, enthaelt alle Infos von Level 1 (APIC Infos, Flags)
 */
cpuid_1 cpuid_level_1()
{
    cpuid_1 result;
    memset(&result, 0, sizeof(result));
    result.eax = 1;
    cpuid(&result.eax, &result.ebx, &result.ecx, &result.edx);
    return result;
}

/**
 * Fragt CPUID Extended Level 0 ab
 *
 * @see cpuid_e0
 * @return cpuid_e0, enthaelt alle Infos von Extended Level 0 (aehnlich Level 0)
 */
cpuid_e0 cpuid_level_e0() 
{
    cpuid_e0 result;
    result.max_level = 0x80000000;
    result.vendorID[12] = '\0';
    cpuid(&result.max_level,
        (uint32_t*)(&result.vendorID[0]),
        (uint32_t*)(&result.vendorID[8]),
        (uint32_t*)(&result.vendorID[4]));
    // Wenn dieser Aufruf schon ungueltig war, (Level garnicht unterstuetzt)
    // Maximalen Extended Level auf 0 setzen, um keine zufaelligen Werte
    // zu hinterlassen.
    if (result.max_level < 0x8000000) {
        result.max_level = 0;
    }
    return result;
}

/**
 * Fragt CPUID Extended Level 1 ab
 *
 * @see cpuid_e1
 * @return cpuid_e1, enthaelt alle Infos von Extended Level 1 (Flags)
 */
cpuid_e1 cpuid_level_e1()
{
    cpuid_e1 result;
    memset(&result, 0, sizeof(result));
    result.eax = 0x80000001;
    cpuid(&result.eax, &result.ebx, &result.ecx, &result.edx);
    return result;
}

/**
 * Fragt CPUID Extended Level 2 bis 4 ab
 * Benennung ist so damit das ganze auch irgendwie zueinander passt.
 *
 * @see cpuid_e2_e4
 * @return cpuid_e2_e4, alle Infos dieser Level (den Processor Brand String)
 */
cpuid_e2_e4 cpuid_level_e2_e4()
{
    cpuid_e2_e4 result;

    result.eax = 0x80000002;
    cpuid(&result.eax,
        (uint32_t*)(&result.proc_brand_string[4]),
        (uint32_t*)(&result.proc_brand_string[8]),
        (uint32_t*)(&result.proc_brand_string[12]));
    *((uint32_t *) &result.proc_brand_string[0]) = result.eax;

    result.eax = 0x80000003;
    cpuid(&result.eax,
        (uint32_t*)(&result.proc_brand_string[20]),
        (uint32_t*)(&result.proc_brand_string[24]),
        (uint32_t*)(&result.proc_brand_string[28]));
    *((uint32_t *) &result.proc_brand_string[16]) = result.eax;

    result.eax = 0x80000004;
    cpuid(&result.eax,
        (uint32_t*)(&result.proc_brand_string[36]),
        (uint32_t*)(&result.proc_brand_string[40]),
        (uint32_t*)(&result.proc_brand_string[44]));
    *((uint32_t *) &result.proc_brand_string[32]) = result.eax;

    result.proc_brand_string[48] = '\0';
    return result;
}
