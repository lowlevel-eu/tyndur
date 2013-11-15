/*
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
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

#include "kernel.h"
#include "paging.h"
#include "string.h"
#include "types.h"


/**
 * Mappt Speicher 1:1 in den Kernel Adressraum. Kein page alignment erforderlich.
 */
bool kernel_identity_map(paddr_t paddr, uint32_t bytes)
{
    uint32_t aligned_addr = (uint32_t)paddr - ((uint32_t)paddr & ~PAGE_MASK);
    uint32_t aligned_bytes = bytes + (uint32_t)paddr -
        ((uint32_t)paddr & PAGE_MASK);

    return map_page_range(kernel_page_directory, (vaddr_t)aligned_addr, (paddr_t)aligned_addr, PTE_W | PTE_P /*| PTE_U*/, NUM_PAGES(aligned_bytes));
}

// Der Teil des Page Directorys, der den Kernelraum mappt und für alle Prozesse gleich ist
#define kernel_page_tables \
    ((uint32_t*)(kernel_page_directory + (KERNEL_BASE >> PGDIR_SHIFT)))
extern page_directory_t kernel_page_directory;
extern uint32_t kernel_pd_id;

/**
 * Errechnet die virtuelle Adresse der Page Table, die zur Adresse addr gehört.
 */
static page_table_t get_kernel_page_table_addr(uint32_t addr)
{
    if(addr < KERNEL_BASE)
    {
        /* TODO: Ein panic mit (einem noch zu schreibenden) BUG()-Makro könnte hier angebrachter sein */
        panic("addr(%x) < KERNEL_BASE", addr);
    }

    return (page_table_t)(KERNEL_PAGE_TABLES_VADDR + (((addr - KERNEL_BASE) >> PGDIR_SHIFT) << PAGE_SHIFT));
}

/**
 * Mappt eine Page Table in die Kernel Page Tables
 *
 * @param pgdir_index Index in das Page Directory. Die 4 MB an der virtuellen
 *    Adresse (pgdir_index << PGDIR_SHIFT) werden von dieser Page Table gemappt.
 * @param addr Physische Adresse der Page Table, die gemappt werden soll
 */
static void map_kernel_page_table(int pgdir_index, uint32_t addr)
{
    // Da alle Page Tables mit (pgdir_index << PGDIR_SHIFT) >= KERNEL_BASE in
    // die Gegend nach KERNEL_PAGE_TABLES_VADDR gemappt werden, müssen wir die
    // dazugehörige Page Table finden.
    page_table_t page_table = get_kernel_page_table_addr(KERNEL_PAGE_TABLES_VADDR);

    // Jede Page Table ist genau 4KB groß, also wird sie mit genau einem Eintrag
    // in der Page Table der Kernel Page Tables gemappt.
    page_table[pgdir_index] = (addr & PAGE_MASK) | PTE_W | PTE_P;

    asm volatile("invlpg %0" : : "m" (*page_table));
}

/**
 * Wandelt eine virtuelle Adresse >= KERNEL_BASE in eine physische
 * Adresse um.
 */
/*static*/ paddr_t kernel_virt_to_phys(uint32_t vaddr)
{
    // Die Page Table für diese virtuelle Adresse ermitteln ...
    page_table_t page_table = get_kernel_page_table_addr(vaddr);

    // ... und auslesen.
    return (paddr_t)(page_table[(vaddr >> PAGE_SHIFT) & 0x3ff] & PAGE_MASK);
}

/**
 * Gibt eine Seite (4 KBytes) im Kernel-Adressraum frei.
 */
void free_kernel_page(vaddr_t page)
{
    uint32_t addr = (uint32_t)page;
    page_table_t page_table;
    
    if(addr < KERNEL_BASE)
    {
        /* TODO: Ein panic mit (einem noch zu schreibenden) BUG()-Makro könnte hier angebrachter sein */
        panic("Es wurde versucht eine Seite ausserhalb des Kernelraums freizugeben.");
    }

    // Die physische Seite vor der virtuellen freigeben, weil sonst kernel_virt_to_phys() nicht mehr funktioniert.
    phys_free_page(kernel_virt_to_phys(addr));

    // Das Mapping aufheben, und die virtuelle Seite somit freigeben.
    page_table = get_kernel_page_table_addr(addr);
    page_table[(addr >> PAGE_SHIFT) & 0x3ff] = 0;
}

/**
 * Findet einen freien Bereich mit num freien Seiten im Kernel-Adressraum.
 *
 * @return Die erste Seite dieses Bereiches
 */
vaddr_t find_contiguous_kernel_pages(int num)
{
    int i, j;
    int pgdir_index, pgtbl_index;
    int num_free_pages;
    page_table_t page_table;
    
    num_free_pages = 0;
    pgdir_index = 0;
    pgtbl_index = 0;

    for(i = 0; i < NUM_KERNEL_PAGE_TABLES; i++)
    {
        if(num_free_pages == 0)
        {
            // Dieser Eintrag im Page Directory wäre der erste von unseren Pages
            pgdir_index = i;
        }

        if(kernel_page_tables[i] & PTE_P)
        {
            // Die virtuelle Adresse der passenden Page Table errechnen
            page_table = (page_table_t)(KERNEL_PAGE_TABLES_VADDR + (i << PAGE_SHIFT));
           
            // Die NULL-Page bleibt grundsätzlich ungemappt, daher wird bei der ersten
            // Page Table erst beim zweiten Eintrag mit Suchen angefangen
            for(j = (i == 0 ? 1 : 0); j < PAGE_TABLE_LENGTH; j++)
            {
                if(!(page_table[j] & PTE_P))
                {
            
                    if(num_free_pages == 0)
                    {
                        pgtbl_index = j;
                    }

                    num_free_pages++;

                    if(num_free_pages >= num)
                    {
                        // Wenn wir genug freie Seiten gefunden haben, die Suche beenden
                        break;
                    }
                }
                else
                {
                    // Eine belegte Page ist uns dazwischen gekommen, und wir müssen mit unserer Suche von vorne anfangen
                    num_free_pages = 0;
                }
            }
        }
        else
        {
            // Diese Page Table ist zwar nicht vorhanden, aber wir können trotzdem mit ihr rechnen

            if(i == 0)
            {
                // Die NULL-Page wird freigehalten, daher muss hier
                // eins von der theoretischen Maximalzahl abgezogen
                // werden
                pgtbl_index = 1;
                num_free_pages += PAGE_TABLE_LENGTH - 1;
            }
            else
            {
                if(num_free_pages == 0)
                {
                    // Diese Page wäre die erste von unseren Pages.
                    pgtbl_index = 0;
                }

                num_free_pages += PAGE_TABLE_LENGTH;
            }
        }

        if(num_free_pages >= num)
        {
            // Wenn wir genug freie Seiten gefunden haben, die Suche beenden
            break;
        }
    }
    
    if(num_free_pages == 0)
    {
        panic("Nicht genug Speicher.");
    }

    return (vaddr_t)(KERNEL_BASE + (pgdir_index << PGDIR_SHIFT) + (pgtbl_index << PAGE_SHIFT));
}

/**
 * Allokiert num_pages Seiten im Kernel-Adressraum. 
 *
 * @return Die Adresse der ersten Seite
 */
vaddr_t alloc_kernel_pages(int num)
{
    int i, j;
    int pgdir_index, pgtbl_index;
    page_table_t page_table;
    vaddr_t vaddr;
    
    vaddr = find_contiguous_kernel_pages(num);
    
    pgdir_index = ((uint32_t)vaddr - KERNEL_BASE) >> PGDIR_SHIFT;
    pgtbl_index = (((uint32_t)vaddr - KERNEL_BASE) >> PAGE_SHIFT) % PAGE_TABLE_LENGTH;

    for(i = pgdir_index; num; i++)
    {
        if(!(kernel_page_tables[i] & PTE_P))
        {
            // Eine neue Page Table muss angelegt werden
            kernel_page_tables[i] = phys_alloc_page() | PTE_W | PTE_P;
            map_kernel_page_table(i, kernel_page_tables[i]);
            page_table = (page_table_t)(KERNEL_PAGE_TABLES_VADDR + (i << PAGE_SHIFT));

            // Page Table mit Nullen initialisieren
            memset(page_table, 0, PAGE_SIZE);

            kernel_pd_id++;
        }
        else
        {
            // Wir verwenden eine bereits vorhandene Page Table
            page_table = (page_table_t)(KERNEL_PAGE_TABLES_VADDR + (i << PAGE_SHIFT)); 
        }

        for(j = (i == pgdir_index) ? pgtbl_index : 0; j < PAGE_TABLE_LENGTH && num; j++, num--)
        {
            page_table[j] = phys_alloc_page() | PTE_W | PTE_P;
            asm volatile("invlpg %0"
                : : "m" (*(char*)((i * PAGE_TABLE_LENGTH + j) * PAGE_SIZE))
                : "memory");
        }
    }

    return vaddr;
}

/**
 * Gibt num_pages Seiten beginnend mit der Adresse first_page im
 * Kernel-Adressraum frei.
 */
void free_kernel_pages(vaddr_t first_page, int num_pages)
{
    int i;
    
    for(i = 0; i < num_pages; i++)
    {
        free_kernel_page((vaddr_t)((uint32_t)first_page + i * PAGE_SIZE));
    }
}


/**
 * Mapt den physikalische Adressbereich irgendwo hin, und gibt einen Pointer 
 * darauf zurueck.
 * @param addr Adresse
 * @param size Groesse des Speicherbereichs
 * @return Pointer auf gemapte stelle
 */
vaddr_t map_phys_addr(paddr_t paddr, size_t size)
{
    // FIXME Diese Funktion sollte nicht auf kernel_page_directory arbeiten, 
    // sondern auf dem aktuellen PD.
    uint32_t aligned_addr = (uint32_t)paddr - ((uint32_t)paddr & ~PAGE_MASK);
    uint32_t aligned_bytes = size + (uint32_t)paddr -
        ((uint32_t)paddr & PAGE_MASK);
    
    vaddr_t vaddr = find_contiguous_kernel_pages(NUM_PAGES(aligned_bytes));
    if (vaddr == NULL) {
        panic("map_phys_addr(0x%08x, 0x%x): vaddr == NULL\n", paddr, size);
        return NULL;
    }
    
    if(!map_page_range(kernel_page_directory, vaddr, (paddr_t)aligned_addr, PTE_W | PTE_P /*| PTE_U*/, NUM_PAGES(aligned_bytes))) {
        panic("map_phys_addr(0x%08x, 0x%x): map_page_rang fehlgeschlagen\n", paddr, size);
        return NULL;
    }
    
    return (vaddr_t)((uint32_t)vaddr + ((uint32_t)paddr & ~PAGE_MASK));
}


/**
 * Free dem physikalische Adressbereich
 * @param addr Adresse
 * @param size Groesse des Speicherbereichs
 */
void free_phys_addr(vaddr_t vaddr, size_t size)
{
    uint32_t aligned_addr = (uint32_t)vaddr - ((uint32_t)vaddr & ~PAGE_MASK);
    uint32_t aligned_bytes = size + (uint32_t)vaddr -
        ((uint32_t)vaddr & PAGE_MASK);
    
    
    map_page_range(kernel_page_directory, (vaddr_t) aligned_addr, (paddr_t)0, 0, NUM_PAGES(aligned_bytes));
}

/**
 * Dient nur als Wrapper fuer malloc()
 */
void* mem_allocate(uint32_t size, uint32_t flags)
{
    return alloc_kernel_pages(PAGE_ALIGN_ROUND_UP(size) >> 12);
}

/**
 * Dient nur als Wrapper fuer malloc()
 */
void mem_free(void* address, uint32_t size)
{
    free_kernel_pages(address, PAGE_ALIGN_ROUND_UP(size) >> 12);
}
