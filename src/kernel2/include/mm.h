/*
 * Copyright (c) 2006-2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Burkhard Weseloh.
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

#ifndef _MM_H_
#define _MM_H_

#include "mm_arch.h"
#include "tasks.h"
//#include "cpu.h"

// Die Anzahl der Pages, die von n Bytes belegt werden.
#define NUM_PAGES(n) ((((n) + ~PAGE_MASK) & PAGE_MASK) / PAGE_SIZE)

// Rundet eine Adresse auf das kleinste Vielfache von PAGE_SIZE > n auf
#define PAGE_ALIGN_ROUND_UP(n) (((n) + ~PAGE_MASK) & PAGE_MASK)

// Rundet eine Adresse auf das gr�te Vielfache von PAGE_SIZE < n ab
#define PAGE_ALIGN_ROUND_DOWN(n) ((n) & PAGE_MASK)


/*
 * Physische Speicherverwaltung
 */

/* Diese beiden werden in kernel.ld definiert. */
extern void kernel_phys_start();
extern void kernel_phys_end();

extern void kernel_start();
extern void kernel_end();


void pmm_init(struct multiboot_info* multiboot_info);

size_t pmm_count_free();
size_t pmm_count_pages();

void* pmm_get_bitmap_start();
size_t pmm_get_bitmap_size();

paddr_t pmm_alloc(size_t num);
paddr_t pmm_alloc_limits(paddr_t lower_limit, paddr_t upper_limit, size_t num);
void pmm_free(paddr_t start, size_t count);

/*
 * Virtuelle Speicherverwaltung
 */
void vmm_init(mmc_context_t* kernel_pd);
void vmm_init_local(mmc_context_t* context);
vaddr_t vmm_kernel_automap(paddr_t start, size_t size);
void vmm_kernel_unmap(vaddr_t start, size_t size);

/*
 * MM-Kontext
 */

mmc_context_t mmc_create();
mmc_context_t mmc_create_kernel_context();

void mmc_destroy(mmc_context_t* context);

bool mmc_map(mmc_context_t* context, vaddr_t vaddr, paddr_t paddr, 
    int flags, size_t num_pages);

bool mmc_unmap(mmc_context_t* context, vaddr_t vaddr, size_t count);
paddr_t mmc_resolve(mmc_context_t* context, vaddr_t vaddr);

vaddr_t mmc_automap(mmc_context_t* context, paddr_t start, size_t count,
    uintptr_t lower_limit, uintptr_t upper_limit, int flags);

void mmc_activate(mmc_context_t* context);

#define mmc_current_context() (cpu_get_current()->thread->process->context)

#endif

