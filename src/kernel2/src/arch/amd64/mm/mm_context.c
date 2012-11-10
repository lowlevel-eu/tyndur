/*  
 * Copyright (c) 2006-2007 The tyndur Project. All rights reserved.
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

#include <string.h>
#include <stdint.h>

#include "kernel.h"
#include "mm.h"
#include "cpu.h"
#include "kprintf.h"
/*
 * Uebersicht ueber die Paging-Strukturen von oben nach unten:
 *  - Page map, abgekuerzt als PM
 *  - Page directory pointer table, abgekuerzt als PDPT
 *  - Page directory, abgekuerzt als PD
 *  - Page table, abgekuerzt als PT
 */

/// Bestimmt ob das NX-Bit benutzt werden soll.
bool mmc_use_no_exec = false;

mmc_context_t kernel_ctx = {
    .version = 0,
    .lock = LOCK_UNLOCKED
};

/**
 * Synchronisiere einen MM-Kontext mit dem Kernel-Kontext
 *
 * @param ctx MM-Kontext
 */
void mmc_sync(mmc_context_t* ctx)
{
    // TODO ctx und kernel_ctx locken (Philosophenproblem)
    if(ctx->version < kernel_ctx.version) {

        mmc_pml4_entry_t* user_pm = MAPPED_PHYS_MEM_GET(ctx->pml4);
        mmc_pml4_entry_t* kernel_pm = MAPPED_PHYS_MEM_GET(kernel_ctx.pml4);

        // Kopiere die vom Kernel belegten Page Maps
        unsigned int kernelStartIndex = PAGE_MAP_INDEX(KERNEL_MEM_START);

        memcpy(&user_pm[kernelStartIndex], &kernel_pm[kernelStartIndex],
            sizeof(mmc_pml4_entry_t) *
                NUM_PAGE_MAPS(KERNEL_MEM_END - KERNEL_MEM_START));


        // Kopiere Identity Map
        unsigned int idMapStartIndex =
            PAGE_MAP_INDEX(MAPPED_PHYS_MEM_START & ~ADDRESS_SIGN_EXTEND);

        memcpy(&user_pm[idMapStartIndex], &kernel_pm[idMapStartIndex],
            sizeof(mmc_pml4_entry_t) *
                NUM_PAGE_MAPS(-1L - MAPPED_PHYS_MEM_START));
        ctx->version = kernel_ctx.version;
    }
}

/**
 * Erstellt einen neuen MM-Kontext (PM) für einen Prozess
 *
 * @return neuer MM-Kontext
 */
mmc_context_t mmc_create()
{
    mmc_context_t context;

    context.version = 0;
    context.lock = LOCK_UNLOCKED;

    // Die Page Map Level 4 initialisieren
    context.pml4 = pmm_alloc(1);
    memset(MAPPED_PHYS_MEM_GET(context.pml4), 0, PAGE_SIZE);

    // Kernel-Bereich mappen
    mmc_sync(&context);

    return context;
}

/**
 * Erstellt einen neuen MM-Kontext für den Kernel.
 * Diese Funktion wird nur zur Initialisierung benutzt, solange kein richtiges
 * Paging läuft. Dies spielt aber bei AMD64 keine Rolle, weil Paging aktiv sein
 * muss, deshalb wird einfach die aktuelle Page Map eingetragen.
 */
mmc_context_t mmc_create_kernel_context()
{
    kernel_ctx.version = 1;
    kernel_ctx.lock = LOCK_UNLOCKED;

    // Die aktuelle Page Map in den Kernel-MM-Kontext übernehmen
    asm volatile ("mov %%cr3, %0" : "=a"(kernel_ctx.pml4));

    return kernel_ctx;
}

/**
 * Eine Page-Table und alle untergeordneten Pages freigeben.
 *
 * @param pd_entry Page-Directory-Eintrag für die freizugebende
 * Page-Table
 */
static void free_pt(mmc_pd_entry_t pd_entry)
{
    if(pd_entry & PAGE_FLAGS_BIG) {
        // 2MiB freigeben
        pmm_free(pd_entry & ~((1L << PAGE_DIRECTORY_SHIFT) - 1),
            PAGE_TABLE_LENGTH);
    } else {
        mmc_pt_entry_t* pt = MAPPED_PHYS_MEM_GET(pd_entry & PAGE_MASK);

        unsigned int i;
        for(i = 0; i < PAGE_TABLE_LENGTH; i++) {
            if(pt[i] != 0) {
                pmm_free(pt[i] & PAGE_MASK, 1);
            }
        }

        pmm_free(pd_entry & PAGE_MASK, 1);
    }
}

/**
 * Ein Page-Directory und alle untergeordneten Strukturen freigeben.
 *
 * @param pdpt_entry Page-Directory-Pointer-Table-Eintrag für das
 * freizugebende Page-Directory
 */
static void free_pd(mmc_pdpt_entry_t pdpt_entry)
{
    if(pdpt_entry & PAGE_FLAGS_BIG) {
        // 1GiB freigeben
        pmm_free(pdpt_entry & ~((1L << PAGE_DIR_PTR_TABLE_SHIFT) - 1),
            PAGE_DIRECTORY_LENGTH * PAGE_TABLE_LENGTH);
    } else {
        mmc_pd_entry_t* pd = MAPPED_PHYS_MEM_GET(pdpt_entry & PAGE_MASK);

        unsigned int i;
        for(i = 0; i < PAGE_DIRECTORY_LENGTH; i++) {
            if(pd[i] != 0) {
                free_pt(pd[i]);
            }
        }

        pmm_free(pdpt_entry & PAGE_MASK, 1);
    }
}

/**
 * Eine Page-Directory-Pointer-Table und alle untergeordneten Strukturen
 * freigeben.
 *
 * @param pml4_entry Page-Map-Eintrag für die freizugebende
 * Page-Directory-Pointer-Table
 */
static void free_pdpt(mmc_pml4_entry_t pml4_entry)
{
    mmc_pdpt_entry_t* pdpt = MAPPED_PHYS_MEM_GET(pml4_entry & PAGE_MASK);

    unsigned int i;
    for(i = 0; i < PAGE_DIR_PTR_TABLE_LENGTH; i++) {
        if(pdpt[i] != 0) {
            free_pd(pdpt[i]);
        }
    }

    pmm_free(pml4_entry & PAGE_MASK, 1);
}

/**
 * Einen MM-Kontext zerstören und den gemappten Userspace-Speicher freigeben.
 *
 * @param context zu zerstörender MM-Kontext
 */
void mmc_destroy(mmc_context_t* context)
{
    mmc_pml4_entry_t* pml4 = MAPPED_PHYS_MEM_GET(context->pml4);

    pml4 += PAGE_MAP_INDEX(USER_MEM_START);
    unsigned int i;
    for(i = 0; i < NUM_PAGE_MAPS(USER_MEM_END - USER_MEM_START);
        i++)
    {
        if(pml4[i] != 0)
            free_pdpt(pml4[i]);
    }

    pmm_free(context->pml4, 1);
}

/**
 * Den Kontext aktivieren indem die Adresse der zugehörigen Page Map in
 * cr3 geladen wird.
 *
 * @param context Kontext
 */
void mmc_activate(mmc_context_t* context)
{
    mmc_sync(context);
    cpu_get_current()->mm_context = context;
    asm("mov %0, %%cr3" : : "r" (context->pml4));
}

/**
 * Pointer auf die Pagemap eines Kontexts holen. Der Pointer ist eine virtuelle
 * Adresse und darf direkt benutzt werden. Wenn er nicht mehr gebraucht wird,
 * muss er per pml4_free freigegeben werden.
 *
 * @param context Kontext
 *
 * @return Die Page Map des Kontexts
 */
static mmc_pml4_entry_t* pml4_get(mmc_context_t* context)
{
   return vmm_kernel_automap(context->pml4, PAGE_SIZE);
}

/**
 * Page Map wieder freigeben
 *
 * @param pm Page Map
 */
static void pml4_free(mmc_pml4_entry_t* pm)
{
    vmm_kernel_unmap(pm, PAGE_SIZE);
}

/**
 * Pointer auf eine Page directory pointer table holen. Weitere Informationen
 * bei pml4_get.
 *
 * @param pm Pointer auf die Pagemap. Dieser muss gemappt sein. Am besten mit
 *              pml4_get.
 * @param create Wenn true, werden nicht existierende PDPTs erstellt und
 *                  gemappt.
 * @param user Wenn die pdpt erstellt werden muss, bestimmt dieser Parameter ob
 *              die pdpt fuer den Usermode bestimmt ist, oder nicht.
 *
 * @return Pointer auf Page directory pointer table, oder NULL wenn sie nicht
 *          existiert, und nicht erstellt werden soll.
 */
static mmc_pdpt_entry_t* pdpt_get(mmc_pml4_entry_t* pm,
    size_t index, bool create, bool user)
{
    // Nur praesente PDPTs mappen
    if ((pm[index] & PAGE_FLAGS_PRESENT) != 0) {
        return vmm_kernel_automap((paddr_t) (pm[index] & PAGE_MASK),
            PAGE_SIZE);
    } else {
        if (create == true) {
            // Speicher allozieren
            paddr_t phys = pmm_alloc(1);
            mmc_pdpt_entry_t* pdpt = vmm_kernel_automap(phys, PAGE_SIZE);
            // Die PDPT wird sauber mit Null initialisiert => Present-Bit
            // geloescht.
            memset(pdpt, 0, PAGE_SIZE);

            // Die pdpt wird in der Pagemap eingetragen
            pm[index] = (mmc_pml4_entry_t) phys | PAGE_FLAGS_PRESENT |
                PAGE_FLAGS_WRITABLE | PAGE_FLAGS_USER;

            return pdpt;
        } else {
            // PDPT Nicht voranden und soll nicht erstellt werden
            return NULL;
        }
    }
}

/**
 * Siehe pml4_free
 */
static void pdpt_free(mmc_pdpt_entry_t* pdpt)
{
    vmm_kernel_unmap(pdpt, PAGE_SIZE);
}

/**
 * Siehe pdpt_get
 */
static mmc_pd_entry_t* pd_get(mmc_pdpt_entry_t* pdpt, size_t index, 
    bool create, bool user)
{
    if ((pdpt[index] & 1) == 1) {
        return vmm_kernel_automap((paddr_t) (pdpt[index] & PAGE_MASK),
            PAGE_SIZE);
    } else {
        if (create == true) {
            // Speicher allozieren
            paddr_t phys = pmm_alloc(1);
            mmc_pd_entry_t* pd = vmm_kernel_automap(phys, PAGE_SIZE);
            // Das PD wird sauber mit Null initialisiert => Present-Bit
            // geloescht.
            memset(pd, 0, PAGE_SIZE);
            
            // Das PD wird in der Pagemap eingetragen
            pdpt[index] = (mmc_pdpt_entry_t) phys | PAGE_FLAGS_PRESENT |
                PAGE_FLAGS_WRITABLE | PAGE_FLAGS_USER;
            
            return pd;
        } else {
            // PD Nicht voranden und soll nicht erstellt werden
            return NULL;
        }

    }
}

/**
 * Siehe pml4_free
 */
static void pd_free(mmc_pd_entry_t* pd)
{
    vmm_kernel_unmap(pd, PAGE_SIZE);
}

/**
 * Siehe pdpt_get
 */
static mmc_pt_entry_t* pt_get(mmc_pd_entry_t* pd, size_t index, bool create,
    bool user)
{
    // Praesent und keine grosse Page
    if (((pd[index] & 1) == 1) && ((pd[index] & (1 << 7)) == 0)) {
        return vmm_kernel_automap((paddr_t) (pd[index] & PAGE_MASK),
            PAGE_SIZE);
    } else {
        if ((create == true) && ((pd[index] & 1) == 0)) {
            // Speicher allozieren
            paddr_t phys = pmm_alloc(1);
            mmc_pt_entry_t* pt = vmm_kernel_automap(phys, PAGE_SIZE);
            // Die PT wird sauber mit Null initialisiert => Present-Bit
            // geloescht.
            memset(pt, 0, PAGE_SIZE);

            // Die PT wird in der Pagemap eingetragen
            pd[index] = (mmc_pd_entry_t) phys | PAGE_FLAGS_PRESENT |
                PAGE_FLAGS_WRITABLE | PAGE_FLAGS_USER;

            return pt;
        } else {
            // PT nicht voranden und soll nicht erstellt werden
            return NULL;
        }
    }
}

/**
 * Siehe pml4_free
 */
static void pt_free(mmc_pt_entry_t* pt)
{
    vmm_kernel_unmap(pt, PAGE_SIZE);
}

/**
 * Das PD aus einem Kontext fuer eine Bestimmte Adresse holen.
 * Bei bedarf kann angegeben werden, dass fehlede Strukturen neu angelegt
 * werden.
 *
 * @param context Kontext aus dem das PD kommen soll
 * @param address Adresse ahnand welcher das PD gesucht werden soll
 * @param create true wenn die Strukturen erstellt werden sollen, false wenn
 *                  nicht.
 * @param user Wenn Stukturen erstellt werden, kann hiermit bestimmt werden, ob
 *              fuer den Usermode bestimmt sein sollen.
 *
 * @return PD
 */
static mmc_pd_entry_t* pd_get_from_context(mmc_context_t* context,
    vaddr_t address, bool create, bool user)
{
    mmc_pd_entry_t* pd = NULL;
    uintptr_t int_address = (uintptr_t) address;

    // Die einzelnen Strukturen nacheinander holen. Dabei wird jeweils nur
    // weiter gefahren, wenn die vorherige geholt werden konnte.
    mmc_pml4_entry_t* pm = pml4_get(context);
    if (pm != NULL) {
        mmc_pdpt_entry_t* pdpt = pdpt_get(pm,
            PAGE_MAP_INDEX(int_address), create, user);

        if (pdpt != NULL) {
            pd = pd_get(pdpt, PAGE_DIR_PTR_TABLE_INDEX(int_address), create,
                user);
            pdpt_free(pdpt);
        }

        pml4_free(pm);
    }

    return pd;
}

/**
 * Die Pagetable aus einem Kontext fuer eine Bestimmte Adresse holen.
 * Bei bedarf kann angegeben werden, dass fehlede Strukturen neu angelegt
 * werden.
 *
 * @param context Kontext aus dem die Page table kommen soll
 * @param address Adresse ahnand welcher die Page table gesucht werden soll
 * @param create true wenn die Strukturen erstellt werden sollen, false wenn
 *                  nicht.
 * @param user Wenn Stukturen erstellt werden, kann hiermit bestimmt werden, ob
 *              fuer den Usermode bestimmt sein sollen.
 *
 * @return Page Table
 */
static mmc_pt_entry_t* pt_get_from_context(mmc_context_t* context,
    vaddr_t address, bool create, bool user)
{
    mmc_pt_entry_t* pt = NULL;
    uintptr_t int_address = (uintptr_t) address;

    // Die einzelnen Strukturen nacheinander holen. Dabei wird jeweils nur
    // weiter gefahren, wenn sie auch voranden ist.
    mmc_pml4_entry_t* pm = pml4_get(context);
    if (pm != NULL) {
        mmc_pdpt_entry_t* pdpt = pdpt_get(pm,
            PAGE_MAP_INDEX(int_address), create, user);

        if (pdpt != NULL) {
            mmc_pd_entry_t* pd = pd_get(pdpt, PAGE_DIR_PTR_TABLE_INDEX(
                int_address) , create, user);

            if (pd != NULL) {
                pt = pt_get(pd, PAGE_DIRECTORY_INDEX(int_address), create,
                    user);

                pd_free(pd);
            }
            pdpt_free(pdpt);
        }

        pml4_free(pm);
    }

    return pt;
}

/**
 * Mappt eine virtuelle Adresse auf eine physische.
 * Beide Adressen muessen dazu 4K-aligned sein.
 *
 * @param context Kontext, in den gemapt werden soll
 * @param vaddr Virtuelle Speicheradresse
 * @param paddr Physische Speicheradresse
 * @param flags Flags, die gesetzt werden sollen
 *
 * @return true, wenn die virtuelle Seite erfolgreich gemappt werden konnte,
 *    false sonst.
 */
static bool map_page
    (mmc_context_t* context, vaddr_t vaddr, paddr_t paddr, int flags)
{
    //kprintf("map 0x%16x => 0x%16x\n", vaddr, paddr);
    bool big_page = false;
    bool user_page = false;
    uint64_t internal_flags = flags;
    
    if ((flags & PAGE_FLAGS_BIG) != 0) {
        big_page = true;
    }

    // Die NULL-Page bleibt aus Sicherheitsgruenden ungemappt
    if (vaddr == NULL) {
        panic("Versuchtes Mapping nach virtuell NULL");
        return false;
    }
    
    // Die Adressen muessen beide auf 4K ausgerichtet sein.
    if ((((((uintptr_t) vaddr | (uintptr_t) paddr)) % PAGE_SIZE) != 0) && 
        (big_page == false))
    {
        panic("Adressen sind nicht 4K-aligned (virt = %x, phys = %x)",
            vaddr, paddr);
        return false;
    } else if ((((((uintptr_t) vaddr | (uintptr_t) paddr)) % (PAGE_SIZE * 512))
        != 0) && (big_page == true))
    {
        panic("Adressen sind nicht 4M-aligned (virt = %x, phys = %x)",
            vaddr, paddr);
    }
    
    // TODO:Flags pruefen.
    if ((internal_flags & PAGE_FLAGS_USER) != 0) {
        user_page = true;
    }
    
    // Wenn das Nx-Bit gesetzt ist aber nicht benutzt werden darf, wird es
    // geloescht. Sonst wird das richtige NX-Bit gesetzt.
    if ((mmc_use_no_exec == false) && ((internal_flags & PAGE_FLAGS_NO_EXEC) !=
        0)) 
    {
        internal_flags &= ~PAGE_FLAGS_NO_EXEC;
    } else if ((mmc_use_no_exec == true) && ((internal_flags &
        PAGE_FLAGS_NO_EXEC) != 0)) 
    {
        internal_flags &= ~PAGE_FLAGS_NO_EXEC;
        internal_flags |= PAGE_FLAGS_INTERNAL_NX;
    }

    // Ab hier wird zwischen grossen und kleinen Pages unterschieden
    if (big_page == true) {
        mmc_pd_entry_t* pd = pd_get_from_context(context, vaddr, true,
            user_page);
        size_t index = PAGE_DIRECTORY_INDEX((uintptr_t) vaddr);

        if (pd == NULL) {
            panic("Doppelbelegung in Page Map. (0x%lx)", (uintptr_t) vaddr);
        }

        if ((pd[index] & 1) == 1) {
            panic("Doppelbelegung im Page Directory. (0x%lx)", (uintptr_t) vaddr);
        }

        pd[index] = (mmc_pd_entry_t) paddr | internal_flags;

        pd_free(pd);
    } else {
        // PT anfordern, oder sie ggf. erstellen 
        mmc_pt_entry_t* pt = pt_get_from_context(context, vaddr, true,
            user_page);
        size_t index = PAGE_TABLE_INDEX((uintptr_t) vaddr);
        // Wenn sie nicht angefordert werden konnte, ist der eintrag im PD
        // durch eine Grosse Page belegt.
        if (pt == NULL) {
            panic("Doppelbelegung im Page Directory. (0x%lx)", (uintptr_t) vaddr);
            return false;
        }

        // Wenn der entsprechende Eintrag in der PT nicht frei ist, wird auch
        // abgebrochen.
        if ((pt[index] & 1) == 1) {
            //kprintf("map 0x%16x => 0x%16x\n", vaddr, paddr);
            panic("Doppelbelegung in einer Page Table. (0x%08x%08x => 0x%08x%08x)", 
                ((uintptr_t) vaddr) >> 32, ((uintptr_t) vaddr) & 0xFFFFFFFF, 
                ((uintptr_t) paddr) >> 32, ((uintptr_t) paddr) & 0xFFFFFFFF);
            return false;
        }
        
        // Wenn alles gut gelaufen ist, wirds jetzt ernst
        pt[index] = (mmc_pt_entry_t) paddr | internal_flags;
        
        pt_free(pt);
    }

    return true;
}

/**
 * Mappt mehrere zusammenhängende virtuelle Seiten auf einen physischen
 * Speicherbereich. Beide Adressen muessen dazu 4K-aligned sein.
 * Wenn 2M-Pages benutzt werden, muessen sie auch 2M-aligned sein ;-).
 * der num parameter soll dabei immernoch die Anzahl der 4K-Seiten beinhalten
 *
 * @param context Kontext, auf den sich die virtuelle
 *    Adresse bezieht
 *
 * @param vaddr Virtuelle Speicheradresse der ersten Page
 * @param paddr Physische Speicheradresse der ersten Page
 * @param flags Flags
 * @param num Anzahl der Seiten
 *
 * @return true, wenn der bereich erfolgreich gemappt werden konnte,
 *    false sonst
 */
bool mmc_map(mmc_context_t* context, vaddr_t vaddr, paddr_t paddr,
    int flags, size_t num_pages)
{
    //kprintf("map(0x%16x, 0x%16x, %d)\n", vaddr, paddr, num_pages);
    size_t i, page_count;
    vaddr_t va;
    paddr_t pa;

    if ((flags & PAGE_FLAGS_BIG) == 0) {
        page_count = 1;
    } else {
        page_count = 512;
    }

    for(i = 0; i < (num_pages / page_count); i++)
    {
        va = (vaddr_t)((uintptr_t)vaddr + i * PAGE_SIZE * page_count);
        pa = (paddr_t)((uintptr_t)paddr + i * PAGE_SIZE * page_count);

        if(map_page(context, va, pa, flags) != true)
        {
            /* TODO: Das bereits geschehene Mapping rückgängig machen */
            return false;
        }
    }

    return true;
}

/**
 * Entfernt das Mapping einer virtuellen Adresse.
 *
 * @param page_directory Page Directory, auf das sich die virtuelle
 *    Adresse bezieht
 * @param vaddr Virtuelle Speicheradresse
 *
 * @return true, wenn die virtuelle Seite erfolgreich ungemappt werden konnte,
 *    false sonst
 */
bool mmc_unmap(mmc_context_t* context, vaddr_t vaddr, size_t count)
{
    kprintf("TODO: unmap(0x%p,%d)\n", vaddr, (int) count);
    //size_t i;

    /*for (i = 0; i < count; i++) {
        if (!unmap_page(page_directory, vaddr + i * PAGE_SIZE)) {
            return false;
        }
    }*/

    return true;
}

