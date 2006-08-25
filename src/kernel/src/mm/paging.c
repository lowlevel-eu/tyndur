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
#include "vmm.h"

page_directory_t kernel_page_directory;

/**
 * Initialisiert die virtuelle Speicherverwaltung. Insbesondere wird ein
 * Page Directory fuer den Kernel angelegt und geladen.
 *
 * Die physische Speicherverwaltung muss dazu bereits initialisiert sein.
 */
void init_paging()
{
    int i;

    // Ein Page Directory anlegen und initialisieren
    kernel_page_directory = (unsigned long*) phys_alloc_page();
    
    // alles 1:1 mappen. present, writeable, user, 4 mb pages 
    // die unteren 4 mb werden nach 0xc0000000 gemappt
    // 
    // FIXME: 0 sollte nicht gemappt werden, damit es bei einem
    // Nullpointerzugriff zuverlässig kracht. Daher sind fuer die
    // unteren 4 MB wohl 4K-Seiten unvermeidlich.
    for(i = 0; i < PAGE_DIRECTORY_LENGTH; i++)
    { 
        kernel_page_directory[i] = (i * PAGE_SIZE) | PTE_P | PTE_W | PTE_U | PTE_PS;
    }

    kernel_page_directory[768] = PTE_P | PTE_W | PTE_U | PTE_PS;

    
    __asm__(
        
        // PSE-Bit setzen (fuer 4M-Pages)
        "movl %%cr4, %%eax\n\t"
        "orl $0x00000010, %%eax\n\t"
        "movl %%eax, %%cr4\n\t"

        // Page directory laden
        "movl %0, %%cr3\n\t"
        
    : : "r"(kernel_page_directory) : "eax");
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
    if (page_directory[vpage / PAGE_TABLE_LENGTH] & PTE_P == 0) {
        
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
    if (page_directory[vpage / PAGE_TABLE_LENGTH] & PTE_P == 0) {
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
