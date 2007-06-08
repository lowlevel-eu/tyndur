/*  
 * Copyright (c) 2006-2007 The LOST Project. All rights reserved.
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

static mmc_context_t current_context();

/// Bestimmt ob das NX-Bit benutzt werden soll.
bool mmc_use_no_exec = FALSE;

/*
#define PAGE_FLAGS_PRESENT  (1 << 0)
#define PAGE_FLAGS_WRITABLE (1 << 1)
#define PAGE_FLAGS_USER     (1 << 2)
#define PAGE_FLAGS_NO_CACHE (1 << 4)
#define PAGE_FLAGS_BIG      (1 << 7)
#define PAGE_FLAGS_GLOBAL   (1 << 8)
#define PAGE_FLAGS_NO_EXEC  (1 << 63)
*/

/**
 * Erstellt einen neuen MM-Kontext (Page Directory) für den Kernel.
 * Diese Funktion wird nur zur Initialisierung benutzt, solange Paging
 * noch nicht aktiviert ist.
 *
 * @return Pointer auf die physikalische Adresse des Kontexts
 */
mmc_context_t mmc_create()
{
    // Die Pagemap initialisieren
    paddr_t phys_page_map = pmm_alloc(1);
    mmc_context_t page_map =
        vmm_kernel_automap(phys_page_map, PAGE_SIZE);

    memset(page_map, 0, PAGE_SIZE);
    vmm_kernel_unmap(page_map, PAGE_SIZE);

    return phys_page_map;
}

/**
 * Erstellt einen neuen MM-Kontext (PM) für den Kernel.
 * Diese Funktion wird nur zur Initialisierung benutzt, solange kein richtiges
 * Paging laeuft. Dies spielt aber bei AMD64 keine Rolle, weil Pagin aktiv sein
 * muss, deshalb wird einfach die normale mmc_create benutzt.
 */
mmc_context_t mmc_create_kernel_context()
{
    mmc_context_t context = mmc_create();
    return context;
}

/**
 * 
 *
 * @return Der aktuelle Kontext
 */
static mmc_context_t current_context()
{
    return (mmc_context_t) cpu_get_context();
}

/*
 *
 * @return Die Page Map des Kontexts
 */
static mmc_pm_entry_t* pm_get(mmc_context_t context)
{
   return vmm_kernel_automap(context, PAGE_SIZE);
}

/**
 * Page Map wieder freigeben
 *
 * @param page_map Page Map
 */
static void pm_free(mmc_pm_entry_t* pm)
{
    vmm_kernel_unmap(pm, PAGE_SIZE);
}

/**
 * @param create Wenn TRUE, werden nicht existierende PDPTs erstellt und
 *                  gemappt.
 * @param user Wenn die pdpt erstellt werden muss,
 */
static mmc_pdpt_entry_t* pdpt_get(mmc_pm_entry_t* pm,
    size_t index, bool create, bool user)
{
    // Nur praesente PDPTs mappen
    if ((pm[index] & PAGE_FLAGS_PRESENT) != 0) {
        return vmm_kernel_automap((paddr_t) (pm[index] & PAGE_MASK), PAGE_SIZE);
    } else {
        if (create == TRUE) {
            // Speicher allozieren
            paddr_t phys = pmm_alloc(1);
            mmc_pdpt_entry_t* pdpt = vmm_kernel_automap(phys, PAGE_SIZE);
            // Die PDPT wird sauber mit Null initialisiert => Present-Bit
            // geloescht.
            memset(pdpt, 0, PAGE_SIZE);
            
            // Die pdpt wird in der Pagemap eingetragen
            pm[index] = (qword) phys | PAGE_FLAGS_PRESENT | 
                PAGE_FLAGS_WRITABLE | PAGE_FLAGS_USER;
            
            return pdpt;
        } else {
            // PDPT Nicht voranden und soll nicht erstellt werden
            return NULL;
        }
    }
}

/**
 *
 */
static void pdpt_free(mmc_pdpt_entry_t* pdpt)
{
    vmm_kernel_unmap(pdpt, PAGE_SIZE);
}

/**
 *
 */
static mmc_pd_entry_t* pd_get(mmc_pdpt_entry_t* pdpt, size_t index, bool create,
    bool user)
{
    if ((pdpt[index] & 1) == 1) {
        return vmm_kernel_automap((paddr_t) (pdpt[index] & PAGE_MASK),
            PAGE_SIZE);
    } else {
        if (create == TRUE) {
            // Speicher allozieren
            paddr_t phys = pmm_alloc(1);
            mmc_pd_entry_t* pd = vmm_kernel_automap(phys, PAGE_SIZE);
            // Das PD wird sauber mit Null initialisiert => Present-Bit
            // geloescht.
            memset(pd, 0, PAGE_SIZE);
            
            // Das PD wird in der Pagemap eingetragen
            pdpt[index] = (qword) phys | PAGE_FLAGS_PRESENT |
                PAGE_FLAGS_WRITABLE | PAGE_FLAGS_USER;
            
            return pd;
        } else {
            // PD Nicht voranden und soll nicht erstellt werden
            return NULL;
        }

    }
}

/**
 *
 */
static void pd_free(mmc_pd_entry_t* pd)
{
    vmm_kernel_unmap(pd, PAGE_SIZE);
}

/**
 *
 */
static mmc_pt_entry_t* pt_get(mmc_pd_entry_t* pd, size_t index, bool create, 
    bool user)
{
    // Praesent und keine grosse Page
    if (((pd[index] & 1) == 1) && ((pd[index] & (1 << 7)) == 0)) {
        return vmm_kernel_automap((paddr_t) (pd[index] & PAGE_MASK), 
            PAGE_SIZE);
    } else {
        if ((create == TRUE) && ((pd[index] & 1) == 0)) {
            // Speicher allozieren
            paddr_t phys = pmm_alloc(1);
            mmc_pt_entry_t* pt = vmm_kernel_automap(phys, PAGE_SIZE);
            // Die PT wird sauber mit Null initialisiert => Present-Bit
            // geloescht.
            memset(pt, 0, PAGE_SIZE);
            
            // Die PT wird in der Pagemap eingetragen
            pd[index] = (qword) phys | PAGE_FLAGS_PRESENT |
                PAGE_FLAGS_WRITABLE | PAGE_FLAGS_USER;

            return pt;
        } else {
            // PT nicht voranden und soll nicht erstellt werden
            return NULL;
        }
    }
}


/**
 *
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
 * @param create TRUE wenn die Strukturen erstellt werden sollen, FALSE wenn
 *                  nicht.
 * @param user Wenn Stukturen erstellt werden, kann hiermit bestimmt werden, ob
 *              fuer den Usermode bestimmt sein sollen.
 *
 * @return PD
 */
static mmc_pd_entry_t* pd_get_from_context(mmc_context_t context, 
    vaddr_t address, bool create, bool user)
{
    mmc_pd_entry_t* pd = NULL;
    uintptr_t int_address = (uintptr_t) address;

    // Die einzelnen Strukturen nacheinander holen. Dabei wird jeweils nur
    // weiter gefahren, wenn sie auch voranden ist.
    mmc_pm_entry_t* pm = pm_get(context);
    if (pm != NULL) {
        mmc_pdpt_entry_t* pdpt = pdpt_get(pm, 
            PAGE_MAP_INDEX(int_address), create, user);

        if (pdpt != NULL) {
            pd = pd_get(pdpt, PAGE_DIR_PTR_TABLE_INDEX(int_address), create, 
                user);
            pdpt_free(pdpt);
        }

        pm_free(pm);
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
 * @param create TRUE wenn die Strukturen erstellt werden sollen, FALSE wenn
 *                  nicht.
 * @param user Wenn Stukturen erstellt werden, kann hiermit bestimmt werden, ob
 *              fuer den Usermode bestimmt sein sollen.
 *
 * @return Page Table
 */
static mmc_pt_entry_t* pt_get_from_context(mmc_context_t context, 
    vaddr_t address, bool create, bool user)
{
    mmc_pt_entry_t* pt = NULL;
    uintptr_t int_address = (uintptr_t) address;

    // Die einzelnen Strukturen nacheinander holen. Dabei wird jeweils nur
    // weiter gefahren, wenn sie auch voranden ist.
    mmc_pm_entry_t* pm = pm_get(context);
    if (pm != NULL) {
        mmc_pdpt_entry_t* pdpt = pdpt_get(pm, 
            PAGE_MAP_INDEX(int_address), create, user);

        if (pdpt != NULL) {
            mmc_pd_entry_t* pd = pd_get(pdpt, PAGE_DIR_PTR_TABLE_INDEX(int_address)
                , create, user);

            if (pd != NULL) {
                pt = pt_get(pd, PAGE_DIRECTORY_INDEX(int_address), create, user);

                pd_free(pd);
            }
            pdpt_free(pdpt);
        }

        pm_free(pm);
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
 * @return TRUE, wenn die virtuelle Seite erfolgreich gemappt werden konnte,
 *    FALSE sonst.
 */
static bool map_page
    (mmc_context_t context, vaddr_t vaddr, paddr_t paddr, int flags)
{
    //kprintf("map 0x%16x => 0x%16x\n", vaddr, paddr);
    bool big_page = FALSE;
    bool user_page = FALSE;
    qword internal_flags = flags;
    
    if ((flags & PAGE_FLAGS_BIG) != 0) {
        big_page = TRUE;
    }

    // Die NULL-Page bleibt aus Sicherheitsgruenden ungemappt
    if (vaddr == NULL) {
        panic("Versuchtes Mapping nach virtuell NULL");
        return FALSE;
    }
    
    // Die Adressen muessen beide auf 4K ausgerichtet sein.
    if ((((((uintptr_t) vaddr | (uintptr_t) paddr)) % PAGE_SIZE) != 0) && 
        (big_page == FALSE))
    {
        panic("Adressen sind nicht 4K-aligned (virt = %x, phys = %x)",
            vaddr, paddr);
        return FALSE;
    } else if ((((((uintptr_t) vaddr | (uintptr_t) paddr)) % (PAGE_SIZE * 512))
        != 0) && (big_page == TRUE))
    {
        panic("Adressen sind nicht 4M-aligned (virt = %x, phys = %x)",
            vaddr, paddr);
    }
    
    // TODO:Flags pruefen.
    if ((internal_flags & PAGE_FLAGS_USER) != 0) {
        user_page = TRUE;
    }
    
    // Wenn das Nx-Bit gesetzt ist aber nicht benutzt werden darf, wird es
    // geloescht. Sonst wird das richtige NX-Bit gesetzt.
    if ((mmc_use_no_exec == FALSE) && ((internal_flags & PAGE_FLAGS_NO_EXEC) != 0)) {
        internal_flags &= ~PAGE_FLAGS_NO_EXEC;
    } else if ((mmc_use_no_exec == TRUE) && ((internal_flags & PAGE_FLAGS_NO_EXEC) != 0)) {
        internal_flags &= ~PAGE_FLAGS_NO_EXEC;
        internal_flags |= PAGE_FLAGS_INTERNAL_NX;
    }

    // Ab hier wird zwischen grossen und kleinen Pages unterschieden
    if (big_page == TRUE) {
        mmc_pd_entry_t* pd = pd_get_from_context(context, vaddr, TRUE,
            user_page);
        size_t index = PAGE_DIRECTORY_INDEX((uintptr_t) vaddr);

        if (pd == NULL) {
            panic("Doppelbelegung in Page Map. (0x%x)", (qword) vaddr);
        }

        if ((pd[index] & 1) == 1) {
            panic("Doppelbelegung im Page Directory. (0x%x)", (qword) vaddr);
        }

        pd[index] = (qword) paddr | internal_flags;

        pd_free(pd);
    } else {
        // PT anfordern, oder sie ggf. erstellen 
        mmc_pt_entry_t* pt = pt_get_from_context(context, vaddr, TRUE,
            user_page);
        size_t index = PAGE_TABLE_INDEX((uintptr_t) vaddr);
        // Wenn sie nicht angefordert werden konnte, ist der eintrag im PD
        // durch eine Grosse Page belegt.
        if (pt == NULL) {
            panic("Doppelbelegung im Page Directory. (0x%x)", (qword) vaddr);
            return FALSE;
        }

        // Wenn der entsprechende Eintrag in der PT nicht frei ist, wird auch
        // abgebrochen.
        if ((pt[index] & 1) == 1) {
            //kprintf("map 0x%16x => 0x%16x\n", vaddr, paddr);
            printf("Doppelbelegung in einer Page Table. (0x%016x => 0x%016x) 0x%016x", 
                vaddr, paddr, pt[index] & PAGE_MASK);
            return FALSE;
        }
        
        // Wenn alles gut gelaufen ist, wirds jetzt ernst
        pt[index] = (qword) paddr | internal_flags;
        /*pt[index] |= 1; //present
        pt[index] |= 4; //user
        pt[index] |= 2; //writable
        pt[index] |= 0; //cache-disable
        pt[index] |= 0; //global
        pt[index] |= 0; //NX*/
        
        pt_free(pt);
    }

    return TRUE;
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
 * @return TRUE, wenn der bereich erfolgreich gemappt werden konnte, 
 *    FALSE sonst
 */
bool mmc_map(mmc_context_t context, vaddr_t vaddr, paddr_t paddr, 
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

        if(map_page(context, va, pa, flags) != TRUE)
        {
            /* TODO: Das bereits geschehene Mapping rückgängig machen */
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * Entfernt das Mapping einer virtuellen Adresse.
 *
 * @param page_directory Page Directory, auf das sich die virtuelle 
 *    Adresse bezieht
 * @param vaddr Virtuelle Speicheradresse
 * 
 * @return TRUE, wenn die virtuelle Seite erfolgreich ungemappt werden konnte, 
 *    FALSE sonst
 */
bool mmc_unmap(mmc_context_t context, vaddr_t vaddr, size_t count) 
{
    kprintf("unmap(0x%16x,%d)\n", vaddr, count);
    //size_t i;

    /*for (i = 0; i < count; i++) {
        if (!unmap_page(page_directory, vaddr + i * PAGE_SIZE)) {
            return FALSE;
        }
    }*/

    return TRUE;
}


