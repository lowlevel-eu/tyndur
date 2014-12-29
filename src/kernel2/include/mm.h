/*
 * Copyright (c) 2006-2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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

#ifndef _MM_H_
#define _MM_H_

#include "mm_arch.h"
#include "tasks.h"
//#include "cpu.h"

/*
 * Physische Speicherverwaltung
 */

/* Diese beiden werden in kernel.ld definiert. */
extern void kernel_phys_start(void);
extern void kernel_phys_end(void);

extern void kernel_start(void);
extern void kernel_end(void);

/* Start des bereichs des Kernels der beschreibbar sein muss */
extern void kernel_rw_start(void);


void pmm_init(struct multiboot_info* multiboot_info);

size_t pmm_count_free(void);
size_t pmm_count_pages(void);

void* pmm_get_bitmap_start(void);
size_t pmm_get_bitmap_size(void);

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

void vmm_userstack_push(pm_thread_t* thread, void* data, size_t size);

/*
 * MM-Kontext
 *
 * FIXME Den ganzen generischen Funktionen i386-spezifische Flags zu geben, ist
 * moeglicherweise keine gute Idee...
 */

mmc_context_t mmc_create(void);
mmc_context_t mmc_create_kernel_context(void);

void mmc_destroy(mmc_context_t* context);

bool mmc_map(mmc_context_t* context, vaddr_t vaddr, paddr_t paddr, 
    int flags, size_t num_pages);

bool mmc_unmap(mmc_context_t* context, vaddr_t vaddr, size_t count);
paddr_t mmc_resolve(mmc_context_t* context, vaddr_t vaddr);

/**
 * Findet einen freien Bereich mit num freien Seiten
 *
 * @return Die Anfangsadresse der ersten Seite dieses Bereiches
 */
vaddr_t mmc_find_free_pages(mmc_context_t* context, size_t num,
    uintptr_t lower_limit, uintptr_t upper_limit);

/**
 * Mappt einen Speicherbereich eines anderen Tasks in einen MM-Kontext an eine
 * gegebene virtuelle Adresse im Zielkontext
 *
 * @param target_ctx Kontext, in den gemappt werden soll
 * @param source_ctx Kontext, aus dem Speicher gemappt werden soll
 * @param start Virtuelle Startadresse des zu mappenden Speicherbereichs
 *      bezueglich source_ctx
 * @param count Anzahl der zu mappenden Seiten
 * @param target_vaddr Virtuelle Startadresse im Zielkontext
 * @param flags Flags fuer die Pagetable
 */
bool mmc_map_user(mmc_context_t* target_ctx, mmc_context_t* source_ctx,
    vaddr_t source_vaddr, size_t count, vaddr_t target_vaddr,
    int flags);

vaddr_t mmc_automap(mmc_context_t* context, paddr_t start, size_t count,
    uintptr_t lower_limit, uintptr_t upper_limit, int flags);
vaddr_t mmc_automap_user(mmc_context_t* target_ctx, mmc_context_t* source_ctx,
    vaddr_t start, size_t count, uintptr_t lower_limit, uintptr_t upper_limit,
    int flags);

/**
 * Alloziert einen virtuell (aber nicht zwingend physisch) zusammenhaengenden
 * Speicherbereich
 *
 * @param context Speicherkontext, in den die Seiten gemappt werden sollen
 * @param num_pages Anzahl der zu mappenden Seiten
 * @param phys_lower_limit Niedrigste zulaessige physische Adresse
 * @param phys_upper_limit Hoechste zulaessige physische Adresse. Wenn der Wert
 * NULL ist, werden die zulaessigen physischen Adressen nicht eingeschraenkt.
 * @param virt_lower_limit Niedrigste zulaessige virtuelle Adresse
 * @param virt_upper_limit Hoechste zulaessige virtuelle Adresse
 * @param flags Flags fuer die Pagetable
 */
vaddr_t mmc_valloc_limits(mmc_context_t* context, size_t num_pages,
    paddr_t phys_lower_limit, paddr_t phys_upper_limit,
    uintptr_t virt_lower_limit, uintptr_t virt_upper_limit, int flags);

/**
 * Alloziert einen virtuell (aber nicht zwingend physisch) zusammenhaengenden
 * Speicherbereich. Dies ist eine abkuerzende Variante von mmc_valloc_limits,
 * die die Standardlimits fuer User- bzw. Kernelspace benutzt, je nachdem ob
 * PTE_U in den Flags gesetzt ist oder nicht.
 *
 * @param context Speicherkontext, in den die Seiten gemappt werden sollen
 * @param num_pages Anzahl der zu mappenden Seiten
 * @param flags Flags fuer die Pagetable
 */
vaddr_t mmc_valloc(mmc_context_t* context, size_t num_pages, int flags);

/**
 * Gibt einen virtuell (aber nicht zwingend physisch) zusammenhaengenden
 * Speicherbereich sowohl virtuell als auch physisch frei.
 *
 * @param context Speicherkontext, aus dem die Seiten freigegeben werden sollen
 * @param vaddr Virtuelle Adresse der ersten freizugebenden Seite
 * @param num_pages Anzahl der freizugebenden Seiten
 */
void mmc_vfree(mmc_context_t* context, vaddr_t vaddr, size_t num_pages);

void mmc_activate(mmc_context_t* context);

#define mmc_current_context() (*cpu_get_current()->mm_context)

/*
 * Shared Memory
 */

/// Initialisiert die SHM-Verwaltung
void shm_init(void);

///  Neuen Shared Memory reservieren
uint32_t shm_create(size_t size);

/// Bestehenden Shared Memory oeffnen.
void* shm_attach(pm_process_t* process, uint32_t id);

/// Shared Memory schliessen.
void shm_detach(pm_process_t* process, uint32_t id);

/// Gibt die Groesse des SHM-Bereichs in Bytes zurueck
size_t shm_size(uint32_t id);

#endif

