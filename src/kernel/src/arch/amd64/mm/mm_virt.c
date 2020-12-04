/*  
 * Copyright (c) 2006-2007 The tyndur Project. All rights reserved.
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

#include "mm.h"
#include "cpu.h"
#include "kernel.h"

extern void pmm_set_bitmap_start(void* bitmap_start);

/// Aus mm_context.c
extern bool mmc_use_no_exec;

/**
 * Initialisiert die virtuelle Speicherverwaltung. Insbesondere wird eine
 * Page Map für den Kernel vorbereitet und geladen.
 *
 * Die physische Speicherverwaltung muss dazu bereits initialisiert sein.
 *
 * @param context Kontext der zur initialisierung benutzt werden soll.
 */
void vmm_init(mmc_context_t* context)
{
    // No execute - Erweiterung aktivieren
    // Mit cpuid wird geprueft, ob sie ueberhaupt verfügbar ist
    uint32_t features = 0;
    asm("movl $0x80000001, %%eax;"
        "cpuid;"
        : "=d" (features) : : "eax", "ebx", "ecx");
    // Wenn das entsprechende Bit gesetzt ist, duerfen wir es gefahrlos
    // benutzen.
    if ((features & (1 << 20)) != 0) {
        mmc_use_no_exec = true;
    } else {
        mmc_use_no_exec = false;
    }
    
    // Den Kernel mappen
    mmc_map(context, kernel_start, (paddr_t) kernel_phys_start, MM_FLAGS_KERNEL_CODE,
        NUM_PAGES((uintptr_t) kernel_end - (uintptr_t) kernel_start));

    // Videospeicher mappen
    mmc_map(context, (vaddr_t) 0xB8000, (paddr_t) 0xB8000, 
        MM_FLAGS_KERNEL_DATA, NUM_PAGES(25*80*2));
    
    // Physikalischen Speicher Mappen
    mmc_map(context, (vaddr_t)(MAPPED_PHYS_MEM_START & (~ADDRESS_SIGN_EXTEND)),
        (paddr_t) 0, MM_FLAGS_KERNEL_DATA | PAGE_FLAGS_BIG, 512 * 1024);

    // Bitmap mit dem physischen Speicher mappen.
    void* phys_mmap = pmm_get_bitmap_start();
    size_t phys_mmap_size = pmm_get_bitmap_size();
    phys_mmap = vmm_kernel_automap((paddr_t) phys_mmap, NUM_PAGES(phys_mmap_size));
    pmm_set_bitmap_start(phys_mmap); 
    
    // Auf dem Bootstrap-Prozessor wird das ganze schon eingerichtet. Diese
    // Funktion muss spaeter auch noch auf den APs aufgerufen werden.
    vmm_init_local(context);
}

/**
 * Erledigt den Teil der Initialisierung, der auf jedem Prozessor erledigt
 * werden muss.
 *
 * @param context Initialisierungscontext
 */
void vmm_init_local(mmc_context_t* context)
{
    // NXE aktivieren.
    if (mmc_use_no_exec == true) {
        cpu_write_msr(0xC0000080, cpu_read_msr(0xC0000080) | (1 << 11));
    }

    // Kontext aktivieren
    mmc_activate(context);
}

/**
 * Mappt physischen Speicher an eine freie Stelle in den Kerneladressraum.
 *
 * @param start Physische Startadresse des zu mappenden Bereichs
 * @param size Größe des zu mappenden Speicherbereichs in Bytes
 */
vaddr_t vmm_kernel_automap(paddr_t start, size_t size) 
{
    // Auf AMD64 werden die ersten 512Gb des physikalischen Speichers direkt
    // mit dem Letzten eintrag in der Page Map gemappt. Dadurch kann einiges an
    // Performance gewonnen werden, die sonst mit mappen und unmappen von
    // temporaeren Pages verbracht wuerde.
    // Das Makro hier, errechnet die Adresse und fuehrt wenn noetig auch sign
    // extension durch.
    return MAPPED_PHYS_MEM_GET(start);
} 

/**
 * Hebt ein Mapping in den Kernelspeicher auf.
 *
 * @param start Virtuelle Startadresse des gemappten Bereichs
 * @param size Größe des zu entmappenden Speicherbereichs in Bytes
 */
void vmm_kernel_unmap(vaddr_t start, size_t size) 
{
    // Siehe Erklaerung in vmm_kernel_automap
}

/**
 * Fuer malloc()
 */
void* mem_allocate(size_t size, int flags)
{
    return vmm_kernel_automap(pmm_alloc(NUM_PAGES(size)), size);
}

void mem_free(vaddr_t vaddr, size_t size)
{
    // TODO
}

