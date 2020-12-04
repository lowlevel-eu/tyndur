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
#include <types.h>
#include <stdint.h>

#include "syscall.h"
#include "kprintf.h"
#include "mm.h"

/**
 * Einen Speicherblock allozieren.
 *
 * @param bytes Groesse des Speicherblocks
 * @param flags Flags
 * @param phys Je nach Flags wird diese Adresse benutzt, um dort die
 *              physische Adresse des Blocks zu speichern.
 *
 * @return Virtuelle Adresse des Blocks
 */
vaddr_t syscall_mem_allocate(size_t bytes, syscall_arg_t flags, paddr_t* phys)
{
    vaddr_t address = NULL;
    size_t page_count = NUM_PAGES(PAGE_ALIGN_ROUND_UP(bytes));

    current_process->memory_used += page_count << PAGE_SHIFT;

    if (flags & 0x80) {
        // ISA-DMA-Speicher
        // Physisch zusammenhaengend und nur in den ersten 16 MB
        // FIXME Eigentlich muesste man hier auf 64k-Grenzen aufpassen
        *phys = pmm_alloc_limits((paddr_t) 0, (paddr_t) (16 * 1024 * 1024),
            page_count);
        address = mmc_automap(&mmc_current_context(), *phys, page_count,
            USER_MEM_START, USER_MEM_END, MM_FLAGS_USER_DATA);
    } else {
        // Normaler Speicher, muss nicht zusammenhaengend sein
        *phys = 0;
        address = mmc_valloc(&mmc_current_context(), page_count,
            MM_FLAGS_USER_DATA);
    }

    return address;
}

/**
 * Einen Speicherblock an bekannter physischer Adresse mappen.
 *
 * @param bytes Groesse des Speicherblocks
 * @param position Physische Adresse des Speicherblocks
 * @param flags Unbenutzt
 *
 * @return Virtuelle Adresse des Blocks
 *
 * --- TODO laut Mail von Freaky ---
 * Hm könnten wir hier vielleicht noch irgendwas einbauen um sicher zu
 * gehen, dass sie dass diese Blocks in der physischen Speicherverwaltung
 * reserviert sind? Ok, jo sie _sollten_ in der Memory-Map reserviert
 * sein, aber irgendwie weiss ich nicht so recht ob wir da nicht besser auf
 * nummer sicher gehen sollten...
 *
 * Das andere Problem wäre SMU wenn ich den Treiber versehtentlich mehrmals
 * starte,oder sogar 2 verschiedene Treiber habe, die gerne den selben
 * Speicherbereich hätten?
 *
 * Aber weit schlimmer könnte das ausgehen, wenn der Prozess sich beendet,
 * oder beendet wird. Dann wird der Speicher doch physisch freigegeben?
 * --- Ende des TODO ---
 */
vaddr_t syscall_mem_allocate_physical(
    size_t bytes, paddr_t position, syscall_arg_t flags)
{
    paddr_t start = PAGE_ALIGN_ROUND_DOWN(position);
    paddr_t end = position + bytes;
    size_t offset = position - start;
    size_t page_count = NUM_PAGES(end - start);

    current_process->memory_used += page_count << PAGE_SHIFT;

    return mmc_automap(&mmc_current_context(), position, page_count,
        USER_MEM_START, USER_MEM_END, MM_FLAGS_USER_DATA) + offset;
}

/**
 * Einen Speicherbereich freigeben
 *
 * @param start Startadresse des Bereichs
 * @param bytes Groesse des Speicherblocks
 */
void syscall_mem_free(vaddr_t start, size_t bytes)
{
    // Adresse auf PAGE_SIZE abrunden
    uintptr_t end = (uintptr_t) start + bytes;
    start = (vaddr_t) PAGE_ALIGN_ROUND_DOWN((uintptr_t) start);
    size_t num_pages = NUM_PAGES(end - (uintptr_t) start);

    current_process->memory_used -= num_pages << PAGE_SHIFT;

    // Jetzt werden die Seiten freigegeben
    while (num_pages-- != 0) {
        // Physische Adresse ausfindig machen und Page in der
        // physischen Speicherverwaltung freigeben
        paddr_t phys = mmc_resolve(&mmc_current_context(), start);
        pmm_free(phys, 1);

        // Page unmappen
        mmc_unmap(&mmc_current_context(), start, 1);

        start = (vaddr_t) ((uintptr_t) start + PAGE_SIZE);
    }
}

/**
 * Einen per syscall_mem_allocate_physical() allozierten Speicherblock wieder
 * freigeben.
 *
 * TODO Prüfen, dass die Page auch wirklich von syscall_mem_allocate_physical()
 * kommt und kein physischer Speicher freigegeben werden muss, sowie dass die
 * Freigabe die gleiche Länge wie die Allokation hat
 * TODO Prüfen, dass die Page auch wirklich gemappt ist und dem Userspace
 * gehört
 */
void syscall_mem_free_physical(vaddr_t start, size_t bytes)
{
    // Adresse auf PAGE_SIZE abrunden
    uintptr_t end = (uintptr_t) start + bytes;
    start = (vaddr_t) PAGE_ALIGN_ROUND_DOWN((uintptr_t) start);
    size_t num_pages = NUM_PAGES(end - (uintptr_t) start);

    current_process->memory_used -= num_pages << PAGE_SHIFT;

    // Jetzt werden die Seiten freigegeben
    mmc_unmap(&mmc_current_context(), start, num_pages);
}

/**
 * Gibt zurueck, wie viele Byte Speicher dem System insgesamt zur Verfuegung
 * stehen und wie viele davon frei sind.
 */
void syscall_mem_info(uint32_t* sum_pages, uint32_t* free_pages)
{
    *sum_pages =  pmm_count_pages() * PAGE_SIZE;
    *free_pages = pmm_count_free() * PAGE_SIZE;
}
