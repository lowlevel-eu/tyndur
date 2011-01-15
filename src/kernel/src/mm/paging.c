/*  
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
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

#include <stdint.h>
#include <stdbool.h>

#include "kernel.h"
#include "string.h"
#include "vmm.h"
#include "paging.h"
#include "kmm.h"
#include "tasks.h"
#include "kprintf.h"


extern unsigned long * phys_mmap;
extern unsigned long phys_mmap_size;

bool map_page(page_directory_t page_directory, vaddr_t vaddr, paddr_t paddr, int flags);
bool map_page_range(page_directory_t page_directory, vaddr_t vaddr, paddr_t paddr, int flags, int num_pages);

page_directory_t kernel_page_directory;
uint32_t kernel_pd_id = 0;

/**
 * Wenn true, werden Page Tables nicht über ihre virtuelle Adresse
 * angesprochen, sondern über die physische. In diesem Fall muß dafür
 * gesorgt sein, daß die Page Table auch unter dieser Adresse zugreifbar
 * ist (z.B. solange Paging nicht aktiviert ist oder per Identity Mapping)
 *
 * Im normalen Betrieb sollte use_phys_page_tables false sein.
 */
bool use_phys_page_tables;


// TODO: PTE_U wird momentan sehr großzügig vergeben, das sollte sich
// irgendwann noch ändern.
    
/**
 * Initialisiert die virtuelle Speicherverwaltung. Insbesondere wird ein
 * Page Directory fuer den Kernel angelegt und geladen.
 *
 * Die physische Speicherverwaltung muss dazu bereits initialisiert sein.
 */
void init_paging(void)
{
    use_phys_page_tables = true;


    // Ein Page Directory anlegen und initialisieren
    kernel_page_directory = (unsigned long*) phys_alloc_page_limit(0x1000);
    memset(kernel_page_directory, 0, PAGE_SIZE);


    /* Kernel Page Tables mappen */
    kernel_page_directory[KERNEL_PAGE_TABLES_VADDR >> PGDIR_SHIFT] = 
        (uint32_t) kernel_page_directory | PTE_W | PTE_P;

    // Den Kernel mappen
    map_page_range(kernel_page_directory, kernel_start, (paddr_t) kernel_phys_start,
        PTE_W | PTE_P /*| PTE_U*/, ((uint32_t)kernel_end - (uint32_t)kernel_start) / PAGE_SIZE);

    // Videospeicher mappen
    map_page_range(kernel_page_directory, (vaddr_t) 0xB8000, (paddr_t)0xB8000, PTE_W | PTE_P /*| PTE_U*/, ((25*80*2) / PAGE_SIZE) + 1);
    
    /* Das Page Directory mappen */
    map_page(kernel_page_directory, kernel_page_directory, (paddr_t)kernel_page_directory, PTE_P | PTE_W);
    
    // Bitmap mit dem physischen Speicher mappen.
    map_page_range(kernel_page_directory, phys_mmap, (paddr_t)phys_mmap, PTE_W | PTE_P,
        (sizeof(uint32_t) * phys_mmap_size + PAGE_SIZE - 1) / PAGE_SIZE);

    __asm__(
        
        // Page directory laden
        "movl %0, %%cr3\n\t"

        // Paging aktivieren
        "mov %%cr0, %%eax\n\t"
        "or $0x80000000, %%eax\n\t"
        "mov %%eax, %%cr0\n\t"
        
    : : "r"(kernel_page_directory) : "eax");
    
    use_phys_page_tables = false;
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
bool map_page_range(page_directory_t page_directory, vaddr_t vaddr, paddr_t paddr, int flags, int num_pages) 
{
    int i;
    vaddr_t va;
    paddr_t pa;

    for(i = 0; i < num_pages; i++)
    {
        va = (vaddr_t)((uint32_t)vaddr + i * PAGE_SIZE);
        pa = (paddr_t)((uint32_t)paddr + i * PAGE_SIZE);

        if(map_page(page_directory, va, pa, flags) != true)
        {
            /* TODO: Das bereits geschehene Mapping rückgängig machen */
            return false;
        }
    }

    return true;
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
bool map_page(page_directory_t page_directory, vaddr_t vaddr, paddr_t paddr, int flags) 
{
    page_table_t page_table;
    
    uint32_t vpage = (uint32_t) vaddr / PAGE_SIZE;

    //kprintf("map_page %x => %x\n", vaddr, paddr);

    // Die NULL-Page bleibt ungemappt
    if (vaddr == NULL) {
        panic("Versuchtes Mapping nach virtuell NULL");
        return false;
    }
  
    if (flags & ~0x01F) {
        // Boese Flags wollen die Adresse manipulieren.
        // Fliegt das entsprechende Programm eben beim naechsten Zugriff
        // auf die Schnauze.
        
        return false;
    }

    if (((uint32_t) vaddr | (uint32_t) paddr) & 0xfff) {
        panic("Adressen sind nicht 4K-aligned (virt = %x, phys = %x)", vaddr, paddr);
    }

    // Wenn es noch keine passende Pagetable gibt, muss eine neue her,
    // ansonsten nehmen wir diese. Und wenn es sich bei dem Eintrag
    // um eine 4M-Seite handelt, laeuft was schief.
    if ((page_directory[vpage / PAGE_TABLE_LENGTH] & PTE_P) == 0) {
        page_table = (page_table_t) phys_alloc_page();
        
        page_directory[vpage / PAGE_TABLE_LENGTH] = ((uint32_t) page_table) | flags | PTE_U;
        
        if (page_directory == kernel_page_directory) {
            if (!use_phys_page_tables) {
                page_table = (page_table_t) (KERNEL_PAGE_TABLES_VADDR + ((sizeof(uint32_t)*vpage) & ~0xfff));
            }
        } else {
            // Die Page Table gehört nicht zum Kernel - folglich sollte man nicht direkt auf
            // ihre Adresse zugreifen, wenn man nicht gerade einen Page Fault auslösen will.
            // Die Page Table muss daher zunächst in den Kerneladressraum gemappt werden.
            vaddr_t kernel_page_table = find_contiguous_kernel_pages(1);
            map_page(kernel_page_directory, kernel_page_table, (paddr_t) page_table, PTE_P | PTE_W);
            
            /* 
            kprintf("Mappe PT in Kerneladressraum; neue PT\n");
            kprintf("PD = %x\n", page_directory);
            kprintf("PT Kernel vaddr = %x, paddr = %x\n", kernel_page_table, page_table);
            */

            page_table = kernel_page_table;
        }

        if (vaddr < (vaddr_t) 0x40000000) {
            kernel_pd_id++;
        }
        
        memset(page_table, 0, PAGE_SIZE);
        
    } else if (page_directory[vpage / PAGE_TABLE_LENGTH] & PTE_PS) {
        panic("Doppelbelegung im Page Directory.");
    } else {
        if (page_directory == kernel_page_directory) {
            if (use_phys_page_tables) {
                page_table = (page_table_t) (page_directory[vpage / PAGE_TABLE_LENGTH] & ~0xFFF);
            } else {
                page_table = (page_table_t) (KERNEL_PAGE_TABLES_VADDR + ((sizeof(uint32_t)*vpage) & ~0xfff));
            }
        } else {
            page_table = (page_table_t) (page_directory[vpage / PAGE_TABLE_LENGTH] & ~0xFFF);
            
            // Die Page Table gehört nicht zum Kernel - folglich sollte man nicht direkt auf
            // ihre Adresse zugreifen, wenn man nicht gerade einen Page Fault auslösen will.
            // Die Page Table muss daher zunächst in den Kerneladressraum gemappt werden.
            
            vaddr_t kernel_page_table = find_contiguous_kernel_pages(1);
            map_page(kernel_page_directory, kernel_page_table, (paddr_t) page_table, PTE_P | PTE_W);
            page_table = kernel_page_table;
        }
    }
    
    // Wenn der Eintrag in der Pagetable noch nicht gesetzt ist, setzen.
    // Ansonsten stehen wir vor einer Doppelbelegung.
    //
    // Wenn genau dasselbe Mapping doppelt gemacht wird, wird darüber hinweggesehen
    //
    // Und wenn das Mapping aufgehoben werden soll, sollte man sich auch nicht darüber
    // beschweren, dass der Eintrag schon besteht.
    if ((page_table[vpage % PAGE_TABLE_LENGTH] & PTE_P) && ((page_table[vpage % PAGE_TABLE_LENGTH] & ~(PTE_A | PTE_D)) != (((uint32_t) paddr) | flags)) && (flags & PTE_P)) {
        panic("Doppelbelegung in einer Page Table.");
        return false;
    } else {
        page_table[vpage % PAGE_TABLE_LENGTH] = ((uint32_t) paddr) | flags;

        if (page_directory != kernel_page_directory) {
            // Falls wir die Page Table extra in den Kerneladressraum gemappt haben,
            // den Speicher wieder freigeben
            unmap_page(kernel_page_directory, page_table);
        }
        // Und falls es um das aktive Page Directory geht, wäre jetzt ein
        // guter Zeitpunkt, den TLB zu invalidieren.
        // FIXME: Im moment auch wenn es sich nicht um das aktuelle PD handelt,
        // aber das sauber zu machen ist zu aufwaendig, da kernel eh ersetzt
        // wird.
        __asm__ __volatile__("invlpg %0" : : "m" (* (char*) vaddr));

        return true;
    }
    
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
bool unmap_page(page_directory_t page_directory, vaddr_t vaddr) 
{
    return map_page(page_directory, vaddr, (paddr_t) NULL, 0);
}

/**
 * Gibt den Pagetable-Eintrag zu einer gegebenen virtuellen Speicheradresse
 * zurueck.
 *
 * @param page_directory Page Directory, auf das sich die Adresse bezieht
 * @param vaddr Aufzuloesende virtuelle Adresse
 *
 * @return Pagetable-Eintrag der 4K-Seite, zu der die gegebene virtuelle
 * Adresse gehoert. 0, wenn die Adresse nicht in einer Seite liegt, die
 * present ist.
 */
static uint32_t get_pagetable_entry
    (page_directory_t page_directory, vaddr_t vaddr)
{
    page_table_t page_table;
    paddr_t phys_page_table;
    uint32_t result;
    
    uint32_t vpage = (uint32_t) vaddr / PAGE_SIZE;
    //kprintf("[Resolv: %x in PD %x]", vaddr, page_directory);

    // Passende Page Table suchen
    // Bei einer 4M-Page sind wir eigentlich schon am Ziel
    if ((page_directory[vpage / PAGE_TABLE_LENGTH] & PTE_P) == 0) {
        return 0;
    } else if (page_directory[vpage / PAGE_TABLE_LENGTH] & PTE_PS) {
        return page_directory[vpage / PAGE_TABLE_LENGTH];
    } else {
        phys_page_table = (paddr_t)
            (page_directory[vpage / PAGE_TABLE_LENGTH] & ~0xFFF);
    }
        
    // Die Page-Table-Adresse ist eine physische Adresse. Am sichersten ist es,
    // die Adresse einfach noch einmal zu mappen.
    page_table = map_phys_addr(phys_page_table, PAGE_SIZE);
    
    // Adresse zusammenbasteln und fertig
    if (page_table[vpage % PAGE_TABLE_LENGTH] & PTE_P) {
        result = page_table[vpage % PAGE_TABLE_LENGTH]; 
    } else {
        result = 0;
    }
        
    // Falls wir die Page Table extra in den Kerneladressraum gemappt haben,
    // den Speicher wieder freigeben
    unmap_page(kernel_page_directory, page_table);

    return result;
}

/**
 * Loest eine virtuelle Adresse bezueglich eines Page Directory
 * in eine physische Adresse auf.
 *
 * @return Physische Adresse oder NULL, wenn die Page nicht vorhanden ist
 */
paddr_t resolve_vaddr(page_directory_t page_directory, vaddr_t vaddr)
{
    uint32_t pte = get_pagetable_entry(page_directory, vaddr);
    
    if ((pte & PTE_P) == 0) {
        return (paddr_t) NULL;
    }

    if ((pte & PTE_PS) == 0) {
        return (paddr_t) ((pte & ~0xFFF) | ((uint32_t) vaddr & 0xFFF));
    } else {
        return (paddr_t) ((pte & ~0x3FFFFF) | ((uint32_t) vaddr & 0x3FFFFF));
    }
}

/**
 * Prueft, ob ein Speicherbereich im Page Directory des aktuellen Tasks
 * komplett gemappt ist und PTE_U (Zugriffsrecht fuer Usermode) gesetzt hat.
 */
bool is_userspace(vaddr_t start, uint32_t len)
{
    vaddr_t cur;

    if (start + len < start) {
        return false;
    }    
    
    for (cur = start; 
        (cur < start + len) && (cur > (vaddr_t) 0xFFF); 
        cur += PAGE_SIZE) 
    {
        uint32_t pte = get_pagetable_entry(current_task->cr3, cur);
        if ((pte == 0) || !(pte & PTE_U)) {
            return false;
        }
    }

    return true;
}


/**
 * Findet einen freien Bereich mit num freien Seiten
 *
 * @return Die Anfangsadresse der ersten Seite dieses Bereiches
 */
vaddr_t find_contiguous_pages(page_directory_t page_directory, int num, uint32_t lower_limit, uint32_t upper_limit)
{
    uint32_t free_pages = 0;
    uint32_t cur_page;
    uint32_t cur_page_table;

    // Die NULL-Page bleibt ungemappt
    if (lower_limit < PAGE_SIZE) {
        lower_limit = PAGE_SIZE;
    }

    cur_page = (lower_limit >> PAGE_SHIFT) % PAGE_TABLE_LENGTH;
    cur_page_table = (lower_limit >> PGDIR_SHIFT);

    while ((free_pages < num) && ((cur_page_table << PGDIR_SHIFT) < upper_limit)) {
        if (page_directory[cur_page_table] & PTE_P) {

            page_table_t page_table;
            vaddr_t kernel_page_table = find_contiguous_kernel_pages(1);

            map_page(kernel_page_directory, kernel_page_table, page_directory[cur_page_table] & PAGE_MASK, PTE_P | PTE_W);
            page_table = kernel_page_table;

            while (cur_page < PAGE_TABLE_LENGTH) {
                if ((page_table[cur_page++] & PTE_P) == 0) {
                    free_pages++;
                } else {
                    free_pages = 0;
                    lower_limit = (cur_page_table << PGDIR_SHIFT) + (cur_page << PAGE_SHIFT);
                }
//                kprintf("{%d-%d-%x}", free_pages, cur_page, lower_limit);
            }

            unmap_page(kernel_page_directory, kernel_page_table);
            
        } else {
            free_pages += PAGE_TABLE_LENGTH;
        }

        cur_page = 0;
        cur_page_table++;
    }

    if ((free_pages >= num) && (lower_limit + (num * PAGE_SIZE) < upper_limit)) {
        return (vaddr_t) lower_limit;
    } else {
        return NULL;
    }
}


/**
 * Vergrössert den Userspace-Stack eines Tasks um pages Seiten
 *
 * @param task_ptr Pointer zur Task-Struktur
 * @param pages Anzahl der zu mappenden Seiten
 */
void increase_user_stack_size(struct task * task_ptr, int pages)
{
	int i;
	for(i = 0; i < pages; i++)
	{
		task_ptr->user_stack_bottom -= PAGE_SIZE;
		if(resolve_vaddr((page_directory_t) task_ptr->cr3, (vaddr_t) task_ptr->user_stack_bottom) != (paddr_t) NULL)
		{
			kprintf("\n"
                "\033[1;37m\033[41m" // weiss auf rot
                "Task gestoppt: Konnte den Stack nicht weiter vergroessern\n"
                "\033[0;37m\033[40m");
            while(1) { asm("hlt"); }
		}

        map_page((page_directory_t) task_ptr->cr3, task_ptr->user_stack_bottom, phys_alloc_page(), PTE_P | PTE_W | PTE_U);
	}
	
}

