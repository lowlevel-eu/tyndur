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

/**
 * Initialisiert die virtuelle Speicherverwaltung. Insbesondere wird ein
 * Page Directory fuer den Kernel angelegt und geladen.
 *
 * Die physische Speicherverwaltung muss dazu bereits initialisiert sein.
 */
void init_paging()
{
    int i;
    page_table_t page_table;

    // Ein Page Directory anlegen und initialisieren
    kernel_page_directory_phys = (unsigned long*) phys_alloc_page();
    
    memset(kernel_page_directory_phys, PAGE_SIZE, 0);
    
    kernel_page_directory_phys[0] = PTE_P | PTE_W | PTE_U | PTE_PS;

//    extern page_table_t kernel_page_tables[NUM_KERNEL_PAGE_TABLES];
//    memcpy(kernel_page_tables, kernel_page_directory, NUM_KERNEL_PAGE_TABLES << 2);

    page_table = phys_alloc_page();
    kernel_page_directory_phys[0x3ff] = (dword)page_table | PTE_W | PTE_P;
    memset(page_table, PAGE_SIZE, 0);

    map_page(kernel_page_directory_phys, (paddr_t)(KERNEL_PAGE_TABLES_VADDR + 0x000ff000), page_table, PTE_P | PTE_W);
    
    page_table = phys_alloc_page();
    kernel_page_directory_phys[0x300] = (dword)page_table | PTE_W | PTE_P;

    for(i = 0; i < PAGE_TABLE_LENGTH; i++)
    {
        page_table[i] = (i << PAGE_SHIFT) | PTE_P | PTE_W | PTE_U;
    }
    
    page_table[0] = (dword)kernel_page_directory_phys | PTE_P | PTE_W | PTE_U;
    kernel_page_directory = (page_directory_t)0xc0000000;

    map_page(kernel_page_directory_phys, (paddr_t)(KERNEL_PAGE_TABLES_VADDR + 0x000c0000), page_table, PTE_P | PTE_W);

   // kernel_page_directory[0x300] = PTE_P | PTE_W | PTE_U | PTE_PS;
    
    __asm__(
        
        // PSE-Bit setzen (fuer 4M-Pages)
        "movl %%cr4, %%eax\n\t"
        "orl $0x00000010, %%eax\n\t"
        "movl %%eax, %%cr4\n\t"

        // Page directory laden
        "movl %0, %%cr3\n\t"
        
    : : "r"(kernel_page_directory_phys) : "eax");
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
