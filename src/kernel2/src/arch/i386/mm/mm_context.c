/*  
 * Copyright (c) 2006-2007 The LOST Project. All rights reserved.
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
#include <lock.h>

#include "kernel.h"
#include "mm.h"
#include "cpu.h"

// FIXME Das darf hier nicht bleiben
//static mmc_context_t* fixme_kernel_page_directory;

/**
 * Aktuelle Version des Kernel Page Directories
 */
dword page_directory_version = 0;
page_directory_t page_directory_current;

/** 
 * Zur Initialisierung: Wenn gesetzt, wird mit nicht mit Paging, sondern 
 * physischen Adressen gearbeitet
 */
bool use_phys_addr = TRUE;

/**
 * Den Kernelspeicher aus dem neusten Kontext korrekt mappen.
 */
static void mmc_sync(mmc_context_t* context);

/**
 * Erstellt einen neuen MM-Kontext (Page Directory)
 */
mmc_context_t mmc_create()
{
    // Das Page Directory initialisieren
    paddr_t phys_page_directory = pmm_alloc(1);
    mmc_context_t context;
    context.version = 0;
    context.lock = LOCK_UNLOCKED;
    context.page_directory = phys_page_directory;
    context.page_directory_virt = vmm_kernel_automap(
        context.page_directory, PAGE_SIZE);

    memset(context.page_directory_virt, 0, PAGE_SIZE);
    
    // Kernel Page Tables mappen
    context.page_directory_virt[PAGETABLES_MEM_START >> PGDIR_SHIFT] = 
        (uintptr_t) context.page_directory | PTE_W | PTE_P;
    
    mmc_sync(&context);
    
    return context;
}

/**
 * Erstellt einen neuen MM-Kontext (Page Directory) für den Kernel.
 * Diese Funktion wird nur zur Initialisierung benutzt, solange Paging
 * noch nicht aktiviert ist.
 */
mmc_context_t mmc_create_kernel_context()
{
    mmc_context_t context;
    // Das Page Directory initialisieren
    page_directory_t page_directory = pmm_alloc(1);
    context.lock = LOCK_UNLOCKED;

    context.page_directory = page_directory;
    context.page_directory_virt = page_directory;

    context.version = page_directory_version;
    memset(page_directory, 0, PAGE_SIZE);

    // Kernel Page Tables mappen
    page_directory[PAGETABLES_MEM_START >> PGDIR_SHIFT] = 
        (uintptr_t)  page_directory | PTE_W | PTE_P;

    // Die Rueckgabe von mmc_current_context muss von Anfang an korrekt sein
    cpu_get_current()->thread->process->context = context;

    //context.page_directory_virt = mmc_automap(&context, context.page_directory,
    //    1, KERNEL_MEM_START, KERNEL_MEM_END, MM_FLAGS_KERNEL_DATA);

    page_directory_current = context.page_directory_virt;
    return context;
}

static void mmc_sync(mmc_context_t* context)
{
    if (context->version < page_directory_version) {
        memcpy(context->page_directory_virt, page_directory_current, 0x3FC);
        context->version = page_directory_version;
    }
}

/**
 * Den Kontext aktivieren indem die Adresse des zugehoerigen Page directorys in
 * cr3 geladen wird.
 *
 * @param context Kontext
 */
void mmc_activate(mmc_context_t* context)
{
    mmc_sync(context);
    asm("movl %0, %%cr3" : : "R" (context->page_directory));
}

/**
 * Kontext zerstoeren und Userspace-Speicher freigeben
 *
 * @param context Kontext
 */
void mmc_destroy(mmc_context_t* context)
{
    // TODO
}

/**
 * Führt ein temporäres Mapping einer Page Table durch, falls sie nicht zum
 * aktuellen Page Directory gehört. Ansonsten wird ein Pointer auf die 
 * Page Table in den oberen 4 MB des Kernelspeichers zurückgegeben
 */
static inline vaddr_t get_pagetable(mmc_context_t* context, size_t index)
{
    page_directory_t pagedir = context->page_directory_virt;
    page_table_t page_table;

    if (context->page_directory == mmc_current_context().page_directory) {
        if (!use_phys_addr) {
            page_table = (page_table_t) 
                (PAGETABLES_MEM_START + (PAGE_TABLE_LENGTH * 4 * index));
        } else {
            page_table = (vaddr_t) (pagedir[index] & PAGE_MASK);
        }
    } else {
        page_table = vmm_kernel_automap
            ((vaddr_t) (pagedir[index] & PAGE_MASK), PAGE_SIZE);
    }

    return page_table;
}

/**
 * Gibt eine mit get_pagetable angeforderte Page Table wieder frei, falls sie
 * nicht zum aktuellen Page Directory gehört.
 */
static inline void free_pagetable(mmc_context_t* context, vaddr_t page_table)
{
    if (context->page_directory != mmc_current_context().page_directory) {
        mmc_unmap(&mmc_current_context(), page_table, 1);
    }
}


/**
 * Mappt eine virtuelle Adresse auf eine physische.
 * Beide Adressen muessen dazu 4K-aligned sein.
 *
 * @param page_directory Page Directory, auf das sich die virtuelle 
 *    Adresse bezieht
 *
 * @param vaddr Virtuelle Speicheradresse
 * @param paddr Physische Speicheradresse
 * @param flags Flags, die in der Page Table gesetzt werden sollen
 * 
 * @return TRUE, wenn die virtuelle Seite erfolgreich gemappt werden konnte, 
 *    FALSE sonst
 */
static bool map_page
    (mmc_context_t* context, vaddr_t vaddr, paddr_t paddr, int flags) 
{


    page_table_t page_table;
    bool clear_page_table = FALSE;
    page_directory_t page_directory = context->page_directory_virt;
    
    dword vpage = (dword) vaddr / PAGE_SIZE;
    
    // kprintf("map_page %x => %x   PD:0x%08x (virt:0x%08x) CPU %u\n", vaddr,
    //     paddr, context->page_directory, context->page_directory_virt,
    //     cpu_get_current()->id);

    // Die NULL-Page bleibt ungemappt
    if (vaddr == NULL) {
        panic("Versuchtes Mapping nach virtuell NULL");
        return FALSE;
    }
  
    // Wenn boese Flags die Adresse manipulieren wollen, fliegt
    // das entsprechende Programm eben beim naechsten Zugriff
    // auf die Schnauze.
    if (flags & ~0x01F) {
        return FALSE;
    }

    // Umgekehrt müssen die Adressen 4K-aligned sein und nicht 
    // irgendwelche Flags enthalten
    if (((dword) vaddr | (dword) paddr) & 0xFFF) {
        panic("Adressen sind nicht 4K-aligned (virt = %x, phys = %x)", 
            vaddr, paddr);
    }
    
    // Falls es sich im den aktuellen Kontext handelt, muss das Page directory
    // nicht gemappt werden, sonst schon.

    // Wenn es noch keine passende Pagetable gibt, muss eine neue her,
    // ansonsten nehmen wir diese. Und wenn es sich bei dem Eintrag
    // um eine 4M-Seite handelt, laeuft was schief.
    lock(&context->lock);
    if ((page_directory[vpage / PAGE_TABLE_LENGTH] & PTE_P) == 0) 
    {
        page_table = (page_table_t) pmm_alloc(1);
        // kprintf("Pagetable=0x%08x\n", page_table);
        page_directory[vpage / PAGE_TABLE_LENGTH] = (dword) page_table | flags;
       
        // An dieser Stelle kann die Page Table noch nicht initialisiert
        // werden, weil sie noch nicht gemappt ist.
        clear_page_table = TRUE;
    } 
    else if (page_directory[vpage / PAGE_TABLE_LENGTH] & PTE_PS) 
    {
        // 4M-Page
        panic("Doppelbelegung im Page Directory. (virt = 0x%x, phys = 0x%x)", vaddr, paddr);
    } 
    unlock(&context->lock);

    // Virtuelle Adresse der Page Table holen
    page_table = get_pagetable(context, vpage / PAGE_TABLE_LENGTH);
    
    lock(&context->lock);
    // Jetzt ist die Page Table gemappt und kann initialisiert werden, falls es
    // sich um eine neu angelegte Page Table handelt.
    if (clear_page_table) {
        memset(page_table, 0, PAGE_SIZE);
    }
    
    // Wenn der Eintrag in der Pagetable noch nicht gesetzt ist, setzen.
    // Ansonsten stehen wir vor einer Doppelbelegung.
    //
    // Wenn genau dasselbe Mapping doppelt gemacht wird, wird darüber 
    // hinweggesehen.
    //
    // Und wenn das Mapping aufgehoben werden soll, sollte man sich auch nicht 
    // darüber beschweren, dass der Eintrag schon besteht.
    
    bool page_is_present = (page_table[vpage % PAGE_TABLE_LENGTH] & PTE_P);
    bool unmap_page = ! (flags & PTE_P);
    bool mapping_changed = 
        ((page_table[vpage % PAGE_TABLE_LENGTH] & ~(PTE_A | PTE_D)) 
        == 
        ((dword) paddr | flags));

    if (page_is_present && !unmap_page && mapping_changed)
    {
        panic("Doppelbelegung in einer Page Table.");
        unlock(&context->lock);
        return FALSE;
    } 
    else 
    {
        // Setze den Page-Table-Eintrag
        page_table[vpage % PAGE_TABLE_LENGTH] = ((dword) paddr) | flags;

        // Falls es um das aktive Page Directory geht, wäre jetzt ein
        // guter Zeitpunkt, den TLB zu invalidieren.
        // Falls wir die Page Table extra in den Kerneladressraum gemappt 
        // haben, den Speicher wieder freigeben.
        if (context->page_directory == mmc_current_context().page_directory) {
            __asm__ __volatile__("invlpg %0" : : "m" (* (char*) vaddr));
        } else {
            mmc_unmap(&mmc_current_context(), page_table, 1);
        }
        
        if (((uintptr_t) vaddr >= KERNEL_MEM_START) && ((uintptr_t) vaddr <
            KERNEL_MEM_END))
        {
            context->version = ++page_directory_version;
            page_directory_current = context->page_directory_virt;
        }
        unlock(&context->lock);
        return TRUE;
    }
}

/**
 * Mappt mehrere zusammenhängende virtuelle Seiten auf einen physischen
 * Speicherbereich. Beide Adressen muessen dazu 4K-aligned sein.
 *
 * @param page_directory Page Directory, auf das sich die virtuelle 
 *    Adresse bezieht
 *
 * @param vaddr Virtuelle Speicheradresse der ersten Page
 * @param paddr Physische Speicheradresse der ersten Page
 * @param flags Flags, die in der Page Table gesetzt werden sollen
 * @param num Anzahl der Seiten
 * 
 * @return TRUE, wenn der bereich erfolgreich gemappt werden konnte, 
 *    FALSE sonst
 */
bool mmc_map(mmc_context_t* context, vaddr_t vaddr, paddr_t paddr, 
    int flags, size_t num_pages) 
{
    // kprintf("vaddr = %x, paddr = %x\n", vaddr, paddr);
    size_t i;
    vaddr_t va;
    paddr_t pa;

    for(i = 0; i < num_pages; i++)
    {
        va = (vaddr_t)((uintptr_t)vaddr + i * PAGE_SIZE);
        pa = (paddr_t)((uintptr_t)paddr + i * PAGE_SIZE);

        if(map_page(context, va, pa, flags) != TRUE)
        {
            // TODO: Das bereits geschehene Mapping rückgängig machen
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
bool mmc_unmap(mmc_context_t* context, vaddr_t vaddr, size_t count) 
{
    size_t i;

    for (i = 0; i < count; i++) {
        if (!map_page(context, vaddr + i * PAGE_SIZE, NULL, 0)) {
            return FALSE;
        }
    }

    return TRUE;
}


/**
 * Loest eine virtuelle Adresse bezueglich eines Page Directory
 * in eine physische Adresse auf.
 *
 * @return Physische Adresse oder NULL, wenn die Page nicht vorhanden ist
 */
paddr_t mmc_resolve(mmc_context_t* context, vaddr_t vaddr)
{
    page_directory_t page_directory = context->page_directory_virt;
    page_table_t page_table;
    paddr_t phys_page_table;
    paddr_t result;
    
    dword vpage = (dword) vaddr / PAGE_SIZE;
    //kprintf("[Resolv: %x in PD %x]", vaddr, page_directory);

    // Passende Page Table suchen
    // Bei einer 4M-Page sind wir eigentlich schon am Ziel
    if ((page_directory[vpage / PAGE_TABLE_LENGTH] & PTE_P) == 0) 
    {
        return NULL;
    } 
    else if (page_directory[vpage / PAGE_TABLE_LENGTH] & PTE_PS) 
    {
        return (paddr_t) (
            (page_directory[vpage / PAGE_TABLE_LENGTH] & ~0x3FFFFF)
          | ((dword) vaddr & 0x3FFFFF));
        
    } 
    else 
    {
        phys_page_table = (page_table_t) 
            (page_directory[vpage / PAGE_TABLE_LENGTH] & ~0xFFF);
    }
        
    // Die Page-Table-Adresse ist eine physische Adresse. Am sichersten ist es,
    // die Adresse einfach noch einmal zu mappen.
    page_table = get_pagetable(context, vpage / PAGE_TABLE_LENGTH);
    
    // Adresse zusammenbasteln und fertig
    if (page_table[vpage % PAGE_TABLE_LENGTH] & PTE_P) {
        
        result = (paddr_t) (
            (page_table[vpage % PAGE_TABLE_LENGTH] & ~0xFFF) 
          | ((dword) vaddr & 0xFFF));
        
    } else {
        result = NULL;
    }
        
    // Falls wir die Page Table extra in den Kerneladressraum gemappt haben,
    // den Speicher wieder freigeben
    free_pagetable(context, page_table);

    return result;
}

/**
 * Findet einen freien Bereich mit num freien Seiten
 *
 * @return Die Anfangsadresse der ersten Seite dieses Bereiches
 */
static vaddr_t find_contiguous_pages
    (mmc_context_t* context, size_t num, 
    uintptr_t lower_limit, uintptr_t upper_limit)
{
    dword free_pages = 0;
    dword cur_page;
    dword cur_page_table;
    page_directory_t page_directory = context->page_directory_virt;
    
    // Die NULL-Page bleibt ungemappt
    if (lower_limit < PAGE_SIZE) {
        lower_limit = PAGE_SIZE;
    }

    // cur_page ist die Schleifenvariable für die Suche.
    // Initialisierung auf die niedrigste erlaubte Page.
    cur_page = (lower_limit >> PAGE_SHIFT) % PAGE_TABLE_LENGTH;
    cur_page_table = (lower_limit >> PGDIR_SHIFT);

    // Suche
    while ((free_pages < num) 
        && ((cur_page_table << PGDIR_SHIFT) < upper_limit)) 
    {
        if (page_directory[cur_page_table] & PTE_P) 
        {
            // Die Page Table ist vorhanden. Wir müssen die einzelnen Einträge
            // durchsuchen, um festzustellen, ob und wieviel Platz darin frei
            // ist.
            page_table_t page_table 
                = get_pagetable(context, cur_page_table);

            while (cur_page < PAGE_TABLE_LENGTH) {
                // Wenn der Eintrag frei ist, haben wir Glück und unseren
                // gefundenen Bereich um eine Page vergrößtert. Wenn nicht, war
                // alles, was wir bisher gefunden haben, umsonst, unnd wir
                // müssen nochmal bei Null anfangen.
                if ((page_table[cur_page++] & PTE_P) == 0) {
                    free_pages++;
                } else {
                    free_pages = 0;
                    lower_limit = (cur_page_table << PGDIR_SHIFT) 
                                + (cur_page << PAGE_SHIFT);
                }
            }

            free_pagetable(context, page_table);
        } 
        else 
        {
            // Die ganze Page Table ist frei und wir haben auf einen Schlag
            // eine ganze Menge freie Seiten gefunden
            free_pages += PAGE_TABLE_LENGTH;
        }

        cur_page = 0;
        cur_page_table++;
    }

    if ((free_pages >= num) && (lower_limit + num * PAGE_SIZE < upper_limit)) {
        return (vaddr_t) lower_limit;
    } else {
        return NULL;
    }
}

/**
 * Mappt einen Speicherbereich in einen MM-Kontext an eine freie Adresse in
 * einem vorgegebenen Adressberech.
 *
 * @param context Kontext, in den gemappt werden soll
 * @param start Physische Startadresse des zu mappenden Speicherbereichs
 * @param count Anzahl der zu mappenden Seiten
 * @param lower_limit Niedrigste zulässige virtuelle Adresse
 * @param upper_limit Höchste zulässige virtuelle Adresse
 * @param flags Flags für die Pagetable
 */
vaddr_t mmc_automap(mmc_context_t* context, paddr_t start, size_t count,
    uintptr_t lower_limit, uintptr_t upper_limit, int flags)
{
    vaddr_t free_page = 
        find_contiguous_pages(context, count, lower_limit, upper_limit);
    
    if (free_page == NULL) {
        return NULL;
    }

    if (mmc_map(context, free_page, start, flags, count)) {
        return free_page;
    } else {
        return NULL;
    }
}

/**
 * Mappt einen Speicherbereich eines anderen Tasks in einen MM-Kontext an eine
 * freie Adresse in einem vorgegebenen Adressberech.
 *
 * @param target_ctx Kontext, in den gemappt werden soll
 * @param source_ctx Kontext, aus dem Speicher gemappt werden soll
 * @param start Virtuelle Startadresse des zu mappenden Speicherbereichs
 *      bezueglich source_ctx
 * @param count Anzahl der zu mappenden Seiten
 * @param lower_limit Niedrigste zulaessige virtuelle Adresse
 * @param upper_limit Hoechste zulaessige virtuelle Adresse
 * @param flags Flags fuer die Pagetable
 */
vaddr_t mmc_automap_user(mmc_context_t* target_ctx, mmc_context_t* source_ctx,
    vaddr_t start, size_t count, uintptr_t lower_limit, uintptr_t upper_limit,
    int flags)
{
    size_t i;
    paddr_t paddr;
    vaddr_t vaddr;
    vaddr_t free_page =
        find_contiguous_pages(target_ctx, count, lower_limit, upper_limit);

    if (free_page == NULL) {
        return NULL;
    }

    // FIXME Hier wird unter Umstaenden ziemlich oft dieselbe Pagetable gemappt
    // und wieder ungemappt (in mmc_resolve)
    for (i = 0; i < count; i++) {
        paddr = mmc_resolve(source_ctx, start + (i * PAGE_SIZE));
        vaddr = free_page + (i * PAGE_SIZE);
        if (!mmc_map(target_ctx, vaddr, paddr, flags, count)) {
            mmc_unmap(target_ctx, free_page, i);
            return NULL;
        }
    }

    return free_page;
}
