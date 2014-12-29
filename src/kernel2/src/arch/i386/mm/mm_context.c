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

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <types.h>
#include <lock.h>

#include "kernel.h"
#include "mm.h"
#include "cpu.h"

// FIXME Das darf hier nicht bleiben
//static mmc_context_t* fixme_kernel_page_directory;

/**
 * Aktuelle Version des Kernel Page Directories
 */
uint32_t page_directory_version = 0;
page_directory_t page_directory_current;

/**
 * Zur Initialisierung: Wenn gesetzt, wird mit nicht mit Paging, sondern
 * physischen Adressen gearbeitet
 */
bool use_phys_addr = true;

/**
 * Den Kernelspeicher aus dem neusten Kontext korrekt mappen.
 */
static void mmc_sync(mmc_context_t* context);

static inline vaddr_t get_pagetable(mmc_context_t* context, size_t index);
static inline void free_pagetable(mmc_context_t* context, vaddr_t page_table);

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
    page_directory_t page_directory = (page_directory_t) pmm_alloc(1);
    context.lock = LOCK_UNLOCKED;

    context.page_directory = (paddr_t) page_directory;
    context.page_directory_virt = page_directory;

    context.version = page_directory_version;
    memset(page_directory, 0, PAGE_SIZE);

    // Kernel Page Tables mappen
    page_directory[PAGETABLES_MEM_START >> PGDIR_SHIFT] =
        (uintptr_t)  page_directory | PTE_W | PTE_P;

    // Die Rueckgabe von mmc_current_context muss von Anfang an korrekt sein
    current_process->context = context;

    //context.page_directory_virt = mmc_automap(&context, context.page_directory,
    //    1, KERNEL_MEM_START, KERNEL_MEM_END, MM_FLAGS_KERNEL_DATA);

    page_directory_current = context.page_directory_virt;
    return context;
}

static void mmc_sync(mmc_context_t* context)
{
    if (context->version < page_directory_version) {
        memcpy(&context->page_directory_virt[1],
               &page_directory_current[1], 0x3FC - 4);
        context->version = page_directory_version;
    }

    // Wenn wir hier das neu synchronisierte PD setzen, können wir Probleme
    // beim zerstoeren von Kontexten umgehen, falls der zu loeschende
    // Kontext auch der aktuellste war.
    page_directory_current = context->page_directory_virt;
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
    cpu_get_current()->mm_context = context;
    asm("movl %0, %%cr3" : : "R" (context->page_directory));
}

/**
 * Kontext zerstoeren und Userspace-Speicher freigeben
 *
 * Achtung: Der noch gemappte Speicher darf nur zu diesem Kontext gehoeren, er
 * wird physisch freigegeben! Shared Memory muss vorher freigegeben werden.
 *
 * @param context Kontext
 */
void mmc_destroy(mmc_context_t* context)
{
    page_directory_t page_directory = context->page_directory_virt;
    page_table_t page_table;
    int i, n;

    // Sicherstellen, dass page_directory_current nicht auf das PD des zu
    // loeschenden Kontexts zeigt.
    mmc_sync(&mmc_current_context());

    for (i = (MM_USER_START >> PGDIR_SHIFT);
         i < (MM_USER_END >> PGDIR_SHIFT);
         i++)
    {
        page_table = get_pagetable(context, i);
        if (page_table == NULL) {
            continue;
        }

        // Alle Page-Table-Eintraege freigeben
        for(n = 0; n < 1024; n++) {
            if(page_table[n] & PTE_P) {
                pmm_free((paddr_t) (page_table[n] & PAGE_MASK), 1);
            }
        }
        free_pagetable(context, page_table);

        // Page Table freigeben
        pmm_free((paddr_t) (page_directory[i] & PAGE_MASK), 1);
    }

    // Page Directory freigeben
    mmc_unmap(&mmc_current_context(), context->page_directory_virt, 1);
    pmm_free(context->page_directory, 1);
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

    if ((pagedir[index] & PTE_P) == 0) {
        return NULL;
    }

    if (context->page_directory == mmc_current_context().page_directory) {
        if (!use_phys_addr) {
            page_table = (page_table_t)
                (PAGETABLES_MEM_START + (PAGE_TABLE_LENGTH * 4 * index));
        } else {
            page_table = (vaddr_t) (pagedir[index] & PAGE_MASK);
        }
    } else {
        page_table = vmm_kernel_automap
            ((paddr_t) (pagedir[index] & PAGE_MASK), PAGE_SIZE);
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
 * @return true, wenn die virtuelle Seite erfolgreich gemappt werden konnte,
 *    false sonst
 */
static bool map_page
    (mmc_context_t* context, vaddr_t vaddr, paddr_t paddr, int flags)
{


    page_table_t page_table;
    bool clear_page_table = false;
    page_directory_t page_directory = context->page_directory_virt;

    uint32_t vpage = (uint32_t) vaddr / PAGE_SIZE;
    bool unmap_page = ! (flags & PTE_P);

    // kprintf("map_page %x => %x   PD:0x%08x (virt:0x%08x) CPU %u\n", vaddr,
    //     paddr, context->page_directory, context->page_directory_virt,
    //     cpu_get_current()->id);

    // Die NULL-Page bleibt ungemappt (außer manchmal)
    if (vaddr == NULL && !unmap_page && !(flags & PTE_ALLOW_NULL)) {
        panic("Versuchtes Mapping nach virtuell NULL");
        return false;
    }
    flags &= ~PTE_ALLOW_NULL;

    // Wenn boese Flags die Adresse manipulieren wollen, fliegt
    // das entsprechende Programm eben beim naechsten Zugriff
    // auf die Schnauze.
    if (flags & ~0x01F) {
        return false;
    }

    // Umgekehrt müssen die Adressen 4K-aligned sein und nicht
    // irgendwelche Flags enthalten
    if (((uint32_t) vaddr | (uint32_t) paddr) & 0xFFF) {
        panic("Adressen sind nicht 4K-aligned (virt = %x, phys = %x)",
            vaddr, paddr);
    }

    // Kernelpages duerfen immer nur im gerade aktiven Kontext gemappt werden.
    // Ales andere ergibt keinen Sinn, weil diese Bereiche ohnehin zwischen
    // allen Kontexten synchron gehalten werden muessen.
    if ((flags & PTE_P) && !(flags & PTE_U) &&
        !(((uintptr_t) vaddr >= KERNEL_MEM_START) &&
          ((uintptr_t) vaddr <  KERNEL_MEM_END)))
    {
        panic("Versuch, Kernelpages in Userspace-Bereich zu mappen: %x",
              vaddr);
    }

    // Außerdem müssen sie im richtigen Speicherbereic gemappt werden
    if ((context != &mmc_current_context()) &&
        ((uintptr_t) vaddr >= KERNEL_MEM_START) &&
        ((uintptr_t) vaddr <  KERNEL_MEM_END))
    {
        panic("Versuch, Kernelpages in inaktivem Kontext zu mappen");
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
        page_directory[vpage / PAGE_TABLE_LENGTH] =
            (uint32_t) page_table | PTE_P | PTE_W | PTE_U;

        // An dieser Stelle kann die Page Table noch nicht initialisiert
        // werden, weil sie noch nicht gemappt ist.
        clear_page_table = true;

        if (((uintptr_t) vaddr >= KERNEL_MEM_START) && ((uintptr_t) vaddr <
            KERNEL_MEM_END) && (context->version != 0xFFFFFFFF))
        {
            context->version = ++page_directory_version;
            page_directory_current = context->page_directory_virt;
        }
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
    bool mapping_changed =
        ((page_table[vpage % PAGE_TABLE_LENGTH] & ~(PTE_A | PTE_D))
        !=
        ((uint32_t) paddr | flags));

    if (page_is_present && !unmap_page && mapping_changed)
    {
        panic("Doppelbelegung in einer Page Table.");
        unlock(&context->lock);
        return false;
    }
    else
    {
        // Setze den Page-Table-Eintrag
        page_table[vpage % PAGE_TABLE_LENGTH] = ((uint32_t) paddr) | flags;

        // Falls es um das aktive Page Directory geht, wäre jetzt ein
        // guter Zeitpunkt, den TLB zu invalidieren.
        // Falls wir die Page Table extra in den Kerneladressraum gemappt
        // haben, den Speicher wieder freigeben.
        if (context->page_directory == mmc_current_context().page_directory) {
            __asm__ __volatile__("invlpg %0" : : "m" (* (char*) vaddr));
        } else {
            mmc_unmap(&mmc_current_context(), page_table, 1);
        }
        unlock(&context->lock);
        return true;
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
 * @return true, wenn der bereich erfolgreich gemappt werden konnte,
 *    false sonst
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

        if(map_page(context, va, pa, flags) != true)
        {
            // TODO: Das bereits geschehene Mapping rückgängig machen
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
    size_t i;

    for (i = 0; i < count; i++) {
        if (!map_page(context, vaddr + i * PAGE_SIZE, (paddr_t) NULL, 0)) {
            return false;
        }
    }

    return true;
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
    paddr_t result;

    uint32_t vpage = (uint32_t) vaddr / PAGE_SIZE;
    //kprintf("[Resolv: %x in PD %x]", vaddr, page_directory);

    // Passende Page Table suchen
    // Bei einer 4M-Page sind wir eigentlich schon am Ziel
    if ((page_directory[vpage / PAGE_TABLE_LENGTH] & PTE_P) == 0)
    {
        return (paddr_t) NULL;
    }
    else if (page_directory[vpage / PAGE_TABLE_LENGTH] & PTE_PS)
    {
        return (paddr_t) (
            (page_directory[vpage / PAGE_TABLE_LENGTH] & ~0x3FFFFF)
          | ((uint32_t) vaddr & 0x3FFFFF));

    }

    // Die Page-Table-Adresse ist eine physische Adresse. Am sichersten ist es,
    // die Adresse einfach noch einmal zu mappen.
    page_table = get_pagetable(context, vpage / PAGE_TABLE_LENGTH);

    // Adresse zusammenbasteln und fertig
    if (page_table[vpage % PAGE_TABLE_LENGTH] & PTE_P) {

        result = (paddr_t) (
            (page_table[vpage % PAGE_TABLE_LENGTH] & ~0xFFF)
          | ((uint32_t) vaddr & 0xFFF));

    } else {
        result = (paddr_t) NULL;
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
vaddr_t mmc_find_free_pages(mmc_context_t* context, size_t num,
    uintptr_t lower_limit, uintptr_t upper_limit)
{
    uint32_t free_pages = 0;
    uint32_t cur_page;
    uint32_t cur_page_table;
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
                    if (free_pages >= num) {
                        break;
                    }
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

    if ((free_pages >= num) && (lower_limit + num * PAGE_SIZE <= upper_limit)) {
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
        mmc_find_free_pages(context, count, lower_limit, upper_limit);

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
    int flags)
{
    size_t i;
    paddr_t paddr;
    vaddr_t vaddr;

    // FIXME Hier wird unter Umstaenden ziemlich oft dieselbe Pagetable gemappt
    // und wieder ungemappt (in mmc_resolve)
    for (i = 0; i < count; i++) {
        paddr = mmc_resolve(source_ctx, source_vaddr + (i * PAGE_SIZE));
        vaddr = target_vaddr + (i * PAGE_SIZE);
        if (!mmc_map(target_ctx, vaddr, paddr, flags, 1)) {
            mmc_unmap(target_ctx, target_vaddr, i);
            return false;
        }
    }

    return true;
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
    vaddr_t free_page =
        mmc_find_free_pages(target_ctx, count, lower_limit, upper_limit);

    if (free_page == NULL) {
        return NULL;
    }

    if (!mmc_map_user(target_ctx, source_ctx, start, count, free_page, flags)) {
        return NULL;
    }

    return free_page;
}

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
inline vaddr_t mmc_valloc_limits(mmc_context_t* context, size_t num_pages,
    paddr_t phys_lower_limit, paddr_t phys_upper_limit,
    uintptr_t virt_lower_limit, uintptr_t virt_upper_limit, int flags)
{
    size_t i;
    paddr_t paddr;
    vaddr_t vaddr;
    vaddr_t free_page = mmc_find_free_pages(context, num_pages,
        virt_lower_limit, virt_upper_limit);

    if (free_page == NULL &&
        !(virt_lower_limit == 0 && virt_upper_limit == num_pages * PAGE_SIZE))
    {
        return NULL;
    }

    for (i = 0; i < num_pages; i++) {
        if (!phys_upper_limit) {
            paddr = pmm_alloc(1);
        } else {
            paddr = pmm_alloc_limits(phys_lower_limit, phys_upper_limit, 1);
        }
        vaddr = free_page + (i * PAGE_SIZE);
        if (!mmc_map(context, vaddr, paddr, flags, 1)) {
            mmc_unmap(context, free_page, i);
            return NULL;
        }
    }

    return free_page;
}

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
vaddr_t mmc_valloc(mmc_context_t* context, size_t num_pages, int flags)
{
    if (flags & PTE_U) {
        return mmc_valloc_limits(context, num_pages, 0, 0,
            MM_USER_START, MM_USER_END, flags);
    } else {
        return mmc_valloc_limits(context, num_pages, 0, 0,
            KERNEL_MEM_START, KERNEL_MEM_END, flags);
    }
}

/**
 * Gibt einen virtuell (aber nicht zwingend physisch) zusammenhaengenden
 * Speicherbereich sowohl virtuell als auch physisch frei.
 *
 * @param context Speicherkontext, aus dem die Seiten freigegeben werden sollen
 * @param vaddr Virtuelle Adresse der ersten freizugebenden Seite
 * @param num_pages Anzahl der freizugebenden Seiten
 */
void mmc_vfree(mmc_context_t* context, vaddr_t vaddr, size_t num_pages)
{
    int i;
    paddr_t paddr;

    for (i = 0; i < num_pages; i++) {
        paddr = mmc_resolve(context, vaddr + (i * PAGE_SIZE));
        pmm_free(paddr, 1);
    }
    mmc_unmap(context, vaddr, num_pages);
}
