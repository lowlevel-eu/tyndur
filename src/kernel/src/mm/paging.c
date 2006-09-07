/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
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

#include "kernel.h"
#include "string.h"
#include "vmm.h"

page_directory_t kernel_page_directory_phys;
page_directory_t kernel_page_directory;

bool map_page(page_directory_t page_directory, vaddr_t vaddr, paddr_t paddr, int flags);
bool map_page_range(page_directory_t page_directory, vaddr_t vaddr, paddr_t paddr, int flags, int num_pages);

/**
 * Initialisiert die virtuelle Speicherverwaltung. Insbesondere wird ein
 * Page Directory fuer den Kernel angelegt und geladen.
 *
 * Die physische Speicherverwaltung muss dazu bereits initialisiert sein.
 */
void init_paging()
{
    page_table_t page_table;


    // Ein Page Directory anlegen und initialisieren
    kernel_page_directory_phys = (unsigned long*) phys_alloc_page();
    memset(kernel_page_directory_phys, PAGE_SIZE, 0);


    /* Kernel Page Tables mappen */

    // Eine Page Table anlegen, initalisieren und im Page Directory eintragen
    page_table = phys_alloc_page();
    memset(page_table, PAGE_SIZE, 0);
    kernel_page_directory_phys[0x3ff] = (dword)page_table | PTE_W | PTE_P;

    // Diese Page Table in die Kernel Page Tables eintragen
    map_page(kernel_page_directory_phys, (paddr_t)(KERNEL_PAGE_TABLES_VADDR + 0x000ff000), page_table, PTE_P | PTE_W);
    

    /* Kernel mappen */

    // Eine Page Table anlegen, initalisieren und im Page Directory eintragen
    page_table = phys_alloc_page();
    memset(page_table, PAGE_SIZE, 0);
    kernel_page_directory_phys[0] = PTE_P | PTE_W | PTE_U | PTE_PS;
    kernel_page_directory_phys[0x300] = (dword)page_table | PTE_W | PTE_P;

    // Den Kernel mappen
    map_page_range(kernel_page_directory_phys, kernel_start, kernel_phys_start, PTE_W | PTE_P, ((dword)kernel_end - (dword)kernel_start) / PAGE_SIZE);

    
    /* Das Page Directory mappen */

    map_page(kernel_page_directory_phys, (paddr_t)(KERNEL_PAGE_TABLES_VADDR + 0x00000000), page_table, PTE_P | PTE_W);

    page_table[0] = (dword)kernel_page_directory_phys | PTE_P | PTE_W | PTE_U;

    
    // TODO: Bitmap mit dem physischen Speicher mappen. Machen wir aber
    // erstmal nicht, da die unteren 4 MB (wo sich die Bitmap befindet)
    // zur Zeit eh noch 1:1 gemappt werden. Es müsste in etwa so aussehen:
    //extern unsigned long * phys_mmap;
    //extern unsigned long phys_mmap_size;
    //map_page_range(kernel_page_directory_phys, 0xfff00000, phys_mmap, PTE_W | PTE_P, phys_mmap_size / PAGE_SIZE + 1);

    __asm__(
        
        // PSE-Bit setzen (fuer 4M-Pages)
        "movl %%cr4, %%eax\n\t"
        "orl $0x00000010, %%eax\n\t"
        "movl %%eax, %%cr4\n\t"

        // Page directory laden
        "movl %0, %%cr3\n\t"
        
    : : "r"(kernel_page_directory_phys) : "eax");

    kernel_page_directory = (page_directory_t)0xc0000000;
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
bool map_page_range(page_directory_t page_directory, vaddr_t vaddr, paddr_t paddr, int flags, int num_pages) 
{
    int i;
    vaddr_t va;
    paddr_t pa;

    for(i = 0; i < num_pages; i++)
    {
        va = (vaddr_t)((dword)vaddr + i * PAGE_SIZE);
        pa = (paddr_t)((dword)paddr + i * PAGE_SIZE);

        if(map_page(page_directory, va, pa, flags) != TRUE)
        {
            /* TODO: Das bereits geschehene Mapping rückgängig machen */
            return FALSE;
        }
    }

    return TRUE;
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
bool map_page(page_directory_t page_directory, vaddr_t vaddr, paddr_t paddr, int flags) 
{
    page_table_t page_table;
    int i;
    
    dword vpage = (dword) vaddr / PAGE_SIZE;
  
    if (flags & ~0x01F) {
        // Boese Flags wollen die Adresse manipulieren.
        // Fliegt das entsprechende Programm eben beim naechsten Zugriff
        // auf die Schnauze.
        
        return FALSE;
    }

    // Wenn es noch keine passende Pagetable gibt, muss eine neue her,
    // ansonsten nehmen wir diese. Und wenn es sich bei dem Eintrag
    // um eine 4M-Seite handelt, laeuft was schief.
    if ((page_directory[vpage / PAGE_TABLE_LENGTH] & PTE_P) == 0) {
        
        page_table = (page_table_t) phys_alloc_page();
        
        for (i = 0; i < PAGE_TABLE_LENGTH; i++) {
            page_table[i] = 0;
        }
        
        page_directory[vpage / PAGE_TABLE_LENGTH] = ((dword) page_table) | flags;
        
    } else if (page_directory[vpage / PAGE_TABLE_LENGTH] & PTE_PS) {
        panic("Doppelbelegung im Page Directory.");
    } else {
        page_table = (page_table_t) (page_directory[vpage / PAGE_TABLE_LENGTH] & ~0xFFF);
    }
    
    //kprintf("map_page %x \n", *page_table);for(;;);
    
    // Wenn der Eintrag in der Pagetable noch nicht gesetzt ist, setzen.
    // Ansonsten stehen wir vor einer Doppelbelegung.
    if (page_table[vpage % PAGE_TABLE_LENGTH] & PTE_P) {
        panic("Doppelbelegung in einer Page Table.");
        return FALSE;
    } else {
        page_table[vpage % PAGE_TABLE_LENGTH] = ((dword) paddr) | flags;
        return TRUE;
    }
    
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
bool unmap_page(page_directory_t page_directory, vaddr_t vaddr) 
{
    return map_page(page_directory, vaddr, NULL, 0);
}


/**
 * Loest eine virtuelle Adresse bezueglich eines Page Directory
 * in eine physische Adresse auf.
 *
 * @return Physische Adresse oder NULL, wenn die Page nicht vorhanden ist
 */
paddr_t resolve_vaddr(page_directory_t page_directory, vaddr_t vaddr)
{
    page_table_t page_table;
    
    dword vpage = (dword) vaddr / PAGE_SIZE;

    // Passende Page Table suchen
    // Bei einer 4M-Page sind wir eigentlich schon am Ziel
    if ((page_directory[vpage / PAGE_TABLE_LENGTH] & PTE_P) == 0) {
        return NULL;
    } else if (page_directory[vpage / PAGE_TABLE_LENGTH] & PTE_PS) {
      
        return (paddr_t) (
            (page_directory[vpage / PAGE_TABLE_LENGTH] & ~0x3FFFFF)
          | ((dword) vaddr & 0x3FFFFF));
        
    } else {
        page_table = (page_table_t) (page_directory[vpage / PAGE_TABLE_LENGTH] & ~0xFFF);
    }
    
    // Adresse zusammenbasteln und fertig
    if (page_table[vpage % PAGE_TABLE_LENGTH] & PTE_P) {
        
        return (paddr_t) (
            (page_table[vpage % PAGE_TABLE_LENGTH] & ~0xFFF) 
          | ((dword) vaddr & 0xFFF));
        
    } else {
        return NULL;
    }
    
}
