/*  
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
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
vaddr_t syscall_mem_allocate(size_t bytes, syscall_arg_t flags, paddr_t phys)
{
    vaddr_t address = NULL;
    size_t page_count = NUM_PAGES(PAGE_ALIGN_ROUND_UP(bytes));

    // Der Speicher muss nicht physisch zusamenhaengend sein.
    paddr_t physical_address = pmm_alloc(page_count);
    address = mmc_automap(&mmc_current_context(), physical_address, page_count,
        USER_MEM_START, USER_MEM_END, MM_FLAGS_USER_DATA);

    return address;
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
    start = (vaddr_t) PAGE_ALIGN_ROUND_DOWN((uintptr_t) start);
    size_t num_pages = NUM_PAGES(bytes);

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
