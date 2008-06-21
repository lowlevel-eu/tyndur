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

#include <stdint.h>

#include "mm.h"
#include "kernel.h"
#include "cpu.h"
#include "apic.h"

extern void pmm_set_bitmap_start(void* bitmap_start);
extern bool use_phys_addr;

/**
 * Initialisiert die virtuelle Speicherverwaltung. Insbesondere wird ein
 * Page Directory fuer den Kernel angelegt und geladen.
 *
 * Die physische Speicherverwaltung muss dazu bereits initialisiert sein.
 */
void vmm_init(mmc_context_t* kernel_context)
{ 
    // TODO: 4M-Pages benutzen, wenn in config.h aktiviert
    // Den Kernel mappen
    mmc_map(kernel_context, kernel_start, kernel_phys_start, PTE_W | PTE_P, 
        NUM_PAGES((uintptr_t) kernel_end - (uintptr_t) kernel_start));
    
    // Videospeicher mappen
    mmc_map(kernel_context, (paddr_t) 0xB8000, (vaddr_t) 0xB8000, PTE_W | PTE_P, 
        NUM_PAGES(25*80*2));
    
    
    //mmc_map(kernel_context, kernel_context->page_directory, kernel_context->page_directory, PTE_P | PTE_W, 1);
    
    // Bitmap mit dem physischen Speicher mappen.
    void* phys_mmap = pmm_get_bitmap_start();
    size_t phys_mmap_size = pmm_get_bitmap_size();
    
    // Hier daft _NICHT vmm_kernel_automap_ benutzt werden, da use_phys_addr
    // hier noch TRUE ist.
    phys_mmap = mmc_automap(kernel_context, phys_mmap, 
        NUM_PAGES(phys_mmap_size), KERNEL_MEM_START, KERNEL_MEM_END, PTE_P | 
        PTE_W);
    pmm_set_bitmap_start(phys_mmap);
    
    //use_phys_addr = FALSE;

    vaddr_t virtual_pd = mmc_automap(kernel_context, kernel_context->
        page_directory, 1, KERNEL_MEM_START, KERNEL_MEM_END,
        MM_FLAGS_KERNEL_DATA);

    apic_map(kernel_context);
    use_phys_addr = FALSE;

    kernel_context->page_directory_virt = virtual_pd;
}

void vmm_init_local(mmc_context_t* kernel_context)
{
    // Und ab ins Verderben
    asm volatile (
        // Adresse des Page directories setzen
        // Der Kommentar weiter oben erklaert auch, warum hier nicht
        // mmc_activate benutzt werden kann.
        "mov %0, %%cr3\n\t"
        // Paging aktivieren
        "mov %%cr0, %%eax\n\t"
        "or $0x80000000, %%eax\n\t"
        "mov %%eax, %%cr0\n\t"
    : : "r" (kernel_context->page_directory) : "eax");
}

/**
 * Mappt physischen Speicher an eine freie Stelle in den Kerneladressraum.
 *
 * @param start Physische Startadresse des zu mappenden Bereichs
 * @param size Größe des zu mappenden Speicherbereichs in Bytes
 */
vaddr_t vmm_kernel_automap(paddr_t start, size_t size) 
{
    if (use_phys_addr == TRUE) {
        return start;
    } else {
        return (vaddr_t) ((uintptr_t) mmc_automap(&mmc_current_context(),
            (paddr_t) ((uintptr_t) start & PAGE_MASK), NUM_PAGES(size),
            KERNEL_MEM_START, KERNEL_MEM_END, PTE_P | PTE_W) | ((uintptr_t)
            start & ~PAGE_MASK));
    }
}

/**
 * Hebt ein Mapping in den Kernelspeicher auf.
 *
 * @param start Virtuelle Startadresse des gemappten Bereichs
 * @param size Größe des zu entmappenden Speicherbereichs in Bytes
 */
void vmm_kernel_unmap(vaddr_t start, size_t size) 
{
    if (use_phys_addr == FALSE) {

        // An Page-Grenzen ausrichten
        size += ((dword) start % PAGE_SIZE);
        start = (vaddr_t) ((dword) start & PAGE_MASK);

        // Eigentliches Unmapping ausfuehren
        if (!mmc_unmap(&mmc_current_context(), start, NUM_PAGES(size))) {
            panic("vmm_kernel_unmap(): Konnte 0x%08x (+ 0x%x) nicht freigeben",
                start, size);
        }
    }
}

#if 0
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
		if(resolve_vaddr((mmc_context_t) task_ptr->cr3, (vaddr_t) task_ptr->user_stack_bottom) != NULL)
		{
			kprintf("\n"
                "\033[1;37m\033[41m" // weiss auf rot
                "Task gestoppt: Konnte den Stack nicht weiter vergroessern\n"
                "\033[0;37m\033[40m");
            while(1) { asm("hlt"); }
		}

        map_page((mmc_context_t) task_ptr->cr3, task_ptr->user_stack_bottom, phys_alloc_page(), PTE_P | PTE_W | PTE_U);
	}
}
#endif

/**
 * Fuer malloc()
 */
void* mem_allocate(dword size, dword flags)
{
    vaddr_t vaddr = vmm_kernel_automap(pmm_alloc(NUM_PAGES(size)), size);
    return vaddr;
}

void mem_free(vaddr_t vaddr, size_t pages)
{
    // TODO
}

