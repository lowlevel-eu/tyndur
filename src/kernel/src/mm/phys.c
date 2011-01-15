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

/*
 * 2006-23-9: malu
 * Dokumentation hinzugefügt
 */

#include <stdint.h>

#include "bitops.h"
#include "kernel.h"
#include "multiboot.h"
#include "vmm.h"
#include "phys.h"
#include "string.h"

extern struct multiboot_info multiboot_info;

/* Diese beiden werden in kernel.ld definiert. */
extern void kernel_phys_start(void);
extern void kernel_phys_end(void);

unsigned long * phys_mmap;
unsigned long phys_mmap_size;
unsigned long phys_mmap_usable_pages;

/**
 * Gibt die Anzahl der freien Pages zurueck
 *
 * @return Anzahl freie Pages
 */
unsigned long phys_count_free_pages()
{
    unsigned long free_pages = 0;
    unsigned long i, j;
    
    for(i = 0; i < phys_mmap_size; i++)
    {
        for(j = 0; j < 32; j++)
        {
            if((phys_mmap[i] & (1 << j)) != 0)
            {
                free_pages++;
            }
        }
    }

    return free_pages;
}

/**
 * Gibt die Anzahl der Pages zurueck
 */
unsigned long phys_count_pages()
{
    return phys_mmap_usable_pages;//ys_mmap_size * 32;
}

/**
 * Markiert eine Page als frei/unbenutzt.
 *
 * @param page Zeiger auf den Anfang der Page die als frei markiert werden soll.
 */

void phys_mark_page_as_free(paddr_t page)
{
//    kprintf("[Free %08x]", page);
    phys_mmap[page / PAGE_SIZE / 32] |= 1 << ((page / PAGE_SIZE) & 31);
}

/**
 * Markiert num Pages als frei/unbenutzt.
 *
  @param page Zeiger auf den Anfang der ersten Page.
 * @param num Anzahl der Pages die als frei markiert werden sollen.
 */

void phys_mark_page_range_as_free(paddr_t page, unsigned int num)
{
    int i;

    for(i = 0; i < num; i++)
    {
        phys_mark_page_as_free(page + i * PAGE_SIZE);
    }
}

/**
 * Markiert eine Page als benutzt.
 *
 * @param page Zeiger auf den Anfang der Page die als benutzt markiert werden soll.
 */

void phys_mark_page_as_used(paddr_t page)
{
    phys_mmap[page / PAGE_SIZE / 32] &= ~(1 << ((page / PAGE_SIZE) & 31));
}

/**
 * Markiert num Pages als benutzt.
 *
 * @param page Zeiger auf den Anfang der ersten Page.
 * @param num Anzahl der Pages die als benutzt markiert werden sollen.
 */

void phys_mark_page_range_as_used(paddr_t page, unsigned int num)
{
    int i;

    for(i = 0; i < num; i++)
    {
        phys_mark_page_as_used(page + i * PAGE_SIZE);
    }
}

/**
 * Sucht eine freie Page und gibt einen Zeiger auf den Anfang zurück.
 *
 * @param lower_limit Mindestgröße der Page.
 *
 * @return Im Erfolgsfall wird ein Zeiger auf den Anfang der Page 
 *      zurückgegeben (Bei Erfolg ist der Rückgabewert immer
 *      durch PAGE_SIZE teilbar). Im Fehlerfall wird 1 zurückgegeben.
 */

paddr_t find_free_page(unsigned long lower_limit)
{
    unsigned int i, j;
    paddr_t page = 0;

    i = lower_limit / PAGE_SIZE / 32;
    if(phys_mmap[i] & (0xffffffff << ((lower_limit / PAGE_SIZE) % 32)))
    {
        j = bit_scan_forward(phys_mmap[i] & (0xffffffff << ((lower_limit / PAGE_SIZE) % 32)));
        page = (i * 32 + j) * PAGE_SIZE;
        return page;
    }

    for(i++; i < phys_mmap_size; i++)
    {
        if(phys_mmap[i])
        {
            j = bit_scan_forward(phys_mmap[i]);
            page = (i * 32 + j) * PAGE_SIZE;
            return page;
        }
    }

    return 1;
}

/**
 * Sucht num freie Pages und gibt einen Zeiger auf den Anfang der ersten Page 
 * zurück.
 *
 * @param lower_limit Mindestgröße jeder Page.
 * @param num Anzahl der Pages.
 *
 * @return Zeiger auf den Anfang der ersten Page.
 * @return Im Erfolgsfall wird ein Zeiger auf den Anfang der ersten 
 *      Page zurückgegeben (Bei Erfolg ist der Rückgabewert immer
 *      durch PAGE_SIZE teilbar). Im Fehlerfall wird 1 zurückgegeben.
 */

/* TODO: unbedingt testen */
paddr_t find_free_page_range(unsigned long lower_limit, unsigned int num)
{
    unsigned int i, j;
    unsigned int found = 0;
    paddr_t page = 0;

    for(i = lower_limit / PAGE_SIZE / 32; i < phys_mmap_size; i++)
    {
        if(phys_mmap[i] == 0)
        {
            found = 0;
            continue;
        }

        if(phys_mmap[i] == 0xffffffff)
        {
            if(found == 0)
            {
                page = i * 32 * PAGE_SIZE;
            }
            found += 32;
        }
        else
        {
            for(j = 0; j < 32; j++)
            {
                if(phys_mmap[i] & (1 << j))
                {
                    if(found == 0)
                    {
                        page = (i * 32 + j) * PAGE_SIZE;
                    }
                    found++;

                    if(found > num)
                    {
                        return page;
                    }
                }
                else
                {
                    found = 0;
                }
            }
        }

        if(found > num)
        {
            return page;
        }
    }

    return 1;
}

/**
 * Markiert eine Page als frei/unbenutzt.
 *
 * @param page Zeiger auf den Anfang der Page die als frei markiert werden soll.
 */

void phys_free_page(paddr_t page)
{
    phys_mark_page_as_free(page);
}

/**
 * Markiert num Pages als frei/unbenutzt.
 *Startp
 * @param page Zeiger auf den Anfang der ersten Page.
 * @param num Anzahl der Pages die als frei markiert werden sollen.
 */

void phys_free_page_range(paddr_t page, unsigned int num)
{
    phys_mark_page_range_as_free(page, num);
}

/**
 * Reserviert eine DMA-Page und markiert sie gleichteitig als benutzt.
 *
 * @return Zeiger auf den Anfang der DMA-Page.
 */

paddr_t phys_alloc_dma_page()
{
    paddr_t page = find_free_page(0);
    if(page & (PAGE_SIZE - 1))
    {
        panic("Kein freier Speicher mehr da.");
    }
    phys_mark_page_as_used(page);
    return page;
}

/**
 * Reserviert eine Page.
 *
 * @return Zeiger auf den Anfang der Page.
 */

paddr_t phys_alloc_page()
{
    paddr_t page = find_free_page(16 * 1024 * 1024);
    if(page & (PAGE_SIZE - 1))
    {
        return phys_alloc_dma_page();
    }
    phys_mark_page_as_used(page);
    return page;
}
   
/**
 * Reserviert eine Page nicht unterhalb einer gegebenen Adresse
 */
paddr_t phys_alloc_page_limit(uint32_t lower_limit)
{
    paddr_t page = find_free_page(16 * 1024 * 1024 > lower_limit ? 16 * 1024 * 1024 : lower_limit);

    if(page & (PAGE_SIZE - 1)) {
        page = find_free_page(lower_limit);
    }

    if(page & (PAGE_SIZE - 1)) {
        panic("Konnte Speicher nicht reservieren");
    }
    phys_mark_page_as_used(page);
    return page;
}


/**
 * Reserviert num DMA-Pages.
 *
 * @return Zeiger auf den Anfang der ersten Page.
 */

paddr_t phys_alloc_dma_page_range(unsigned int num)
{
    paddr_t page = find_free_page_range(0, num);
    if((uint32_t) page & (PAGE_SIZE - 1))
    {
        panic("Keine freier Speicher mehr da.");
    }
    phys_mark_page_range_as_used(page, num);
    return page;
}

/**
 * Reserviert num Pages.
 *
 * @return Zeiger auf den Anfang der ersten Page.
 */

paddr_t phys_alloc_page_range(unsigned int num)
{
    paddr_t page = find_free_page_range(16 * 1024 * 1024, num);
    if((uint32_t) page & (PAGE_SIZE - 1))
    {
        return phys_alloc_dma_page_range(num);
    }
    phys_mark_page_range_as_used(page, num);
    return page;
}

/**
 * Reserviert num DMA-Pages, die keine 64k-Grenzen enthalten
 *
 * @return Zeiger auf den Anfang der ersten Page.
 */

paddr_t phys_alloc_dma_page_range_64k(unsigned int num)
{
    uint32_t pos = 0;
    paddr_t page;

    while(1)
    {
        page = find_free_page_range(pos, num);

        if(page & (PAGE_SIZE - 1))
        {
            panic("Keine freier Speicher mehr da.");
        }

        pos = (uint32_t) page;
        if ((pos % 65536) + (num * PAGE_SIZE) > 65536) {
            //printf("Kernel: dma_64k: %x + %x ueberschreitet 64k-Grenze\n",
            //    pos, num);
            pos = (pos + 32 * PAGE_SIZE);
        } else {
            break;
        }
    } 

    phys_mark_page_range_as_used(page, num);
    return page;
}

/**
 * Initialisiert den physischen Speicher.
 *
 * @param mmap_addr Adresse der Memorymap.
 * @param mmap_length Länge der Memorymap.
 * @param upper_mem Größe des upper_mem in Kilobyte, wie in der Multboot-Info
 *      übergeben.
 */

void init_phys_mem(vaddr_t mmap_addr, uint32_t mmap_length, uint32_t upper_mem)
{
    /* freien physischen speicher ermitteln */
    struct multiboot_mmap * mmap;
    struct {
        uint32_t start;
        uint32_t end;
    } available_memory[16 + (3 * multiboot_info.mi_mods_count)];
    int memblocks_count = 0;
    int i, j;
    uint32_t upper_end = 0;
    phys_mmap_usable_pages = 0;
    /* Die von GRUB gelieferte memory map in unser Format umwandeln. */
    if (mmap_length) {
        for(mmap = (struct multiboot_mmap*)mmap_addr;
            mmap < (struct multiboot_mmap*)(mmap_addr + mmap_length);
            mmap = (struct multiboot_mmap*)((char*)mmap + mmap->mm_size + 4))
        {
            if(mmap->mm_type == 1)
            {
                available_memory[memblocks_count].start = PAGE_ALIGN_ROUND_UP(mmap->mm_base_addr);
                available_memory[memblocks_count].end = PAGE_ALIGN_ROUND_DOWN(mmap->mm_base_addr + mmap->mm_length);

                phys_mmap_usable_pages += (available_memory[memblocks_count].end - available_memory[memblocks_count].start) / PAGE_SIZE;

                memblocks_count++;
            }
        }
    }
    else
    {
        // Wenn GRUB keine Memory Map übergibt, Defaultwerte annehmen
        // Dabei vertrauen wir darauf, daß zumindest upper_mem korrekt
        // gesetzt ist.
        available_memory[0].start = 0x0;
        available_memory[0].end = 0x9fc00;
        
        available_memory[1].start = 0x100000;
        available_memory[1].end = 0x100000 + (1024 * upper_mem);
        
        phys_mmap_usable_pages += (available_memory[0].end - available_memory[0].start) / PAGE_SIZE;
        phys_mmap_usable_pages += (available_memory[1].end - available_memory[1].start) / PAGE_SIZE;
        
        memblocks_count = 2;
    }

    /* In der Liste der physischen Speicherblöcke (im folgenden einfach
       Blöcke bzw. Block) einen Bereich, der von start und end angegeben wird,
       als nicht verfügbar definieren.

       Diese Funktion wird durch die Annahmen vereinfacht, dass GRUB den
       Anfang und das Ende eines Moduls immer in genau einen Speicherblock lädt.
       Außerdem dadurch, dass die Liste der verfügbaren Speicherblöcke nicht
       sortiert sein muss.
     */
    void carve(uint32_t start, uint32_t end)
    {
        uint32_t i;

        start = PAGE_ALIGN_ROUND_DOWN(start);
        end = PAGE_ALIGN_ROUND_UP(end);

        //printf("carve: %x bis %x\n", start, end);
        
        for(i = 0; i < memblocks_count; i++)
        {
            if(start == available_memory[i].start)
            {
                /* Der Bereich beginnt genau an einem Blockanfang */

                if(end < available_memory[i].end)
                {
                    /* Der Bereich endet mittem im Block */

                    /* Den Beginn des Blocks hinter den Bereich verschieben */
                    available_memory[i].start = end;
                    break;
                }
                else if(end == available_memory[i].end)
                {
                    /* Der Bereich nimmt den ganzen Block ein */

                    /* Den Block aus der Liste entfernen */
                    memblocks_count--;
                    for(j = i; j < memblocks_count; j++)
                    {
                        available_memory[j].start = available_memory[j + 1].start;
                        available_memory[j].end = available_memory[j + 1].end;
                    }
                    break;
                }
            }
            else if(start > available_memory[i].start)
            {
                /* Der Bereich beginnt irgendwo im Block */

                if(end < available_memory[i].end)
                {
                    /* Der Bereich endet irgendwo mitten im Block. Das bedeutet
                       ein Block muss in der Mitte zerteilt werden. Dazu muss
                       ein weiterer Eintrag in der Liste angelegt werden. */

                    /* Einen neuen Eintrag am Ende anlegen */
                    available_memory[memblocks_count].start = end;
                    available_memory[memblocks_count].end = available_memory[i].end;

                    /* Den Eintrag verkleinern */
                    // available_memory[i].start bleibt unverändert
                    available_memory[i].end = start;

                    memblocks_count++;
                    break;
                }
                else if(end == available_memory[i].end)
                {
                    /* Der Bereich endet genau mit dem Block */

                    /* Das Ende des Bereichs auf den Anfang des Blocks setzen. */
                    available_memory[i].end = start;
                    break;
                }

                /* Wenn sowohl start als auch end hinter dem Speicherblock
                   liegen, wird nichts gemacht. */
            }

            /* Wenn der start vor dem Speicherblock liegt, wird nichts gemacht,
               denn wir gehen davon aus, dass GRUB kein Modul in einen Bereich
               lädt, der kein verfügbarer Speicher ist. */
        }
    }

    carve((uint32_t)kernel_phys_start, (uint32_t)kernel_phys_end);

    /* foreach(modul in module) { carve(modul.start, module.end); } // pseudocode btw ;) */    
    {
        uint32_t i;
        struct multiboot_module * multiboot_module;
        multiboot_module = multiboot_info.mi_mods_addr;

        for (i = 0; i < multiboot_info.mi_mods_count; i++) {
            carve((uint32_t) multiboot_module, ((uint32_t) multiboot_module) + sizeof(multiboot_module));
            carve((uint32_t) multiboot_module->start, (uint32_t) multiboot_module->end);
            if (multiboot_module->cmdline) {
                carve((uint32_t) multiboot_module->cmdline, (uint32_t) multiboot_module->cmdline + strlen(multiboot_module->cmdline));
            }

            multiboot_module++;
        }
    }

    /* Die obere Grenze des physischen Speichers ermitteln. */
    for(i = 0; i < memblocks_count; i++)
    {
        if(available_memory[i].end > upper_end)
        {
            upper_end = available_memory[i].end;
        }
    }
    
    /* einen ort für die tabelle mit den physischen seiten suchen */
    phys_mmap_size = upper_end / PAGE_SIZE / 32;
    for(i = 0; i < memblocks_count; i++)
    {
        if((available_memory[i].start) && (available_memory[i].start + phys_mmap_size * 4 < available_memory[i].end))
        {
            phys_mmap = (unsigned long*)available_memory[i].start;
            available_memory[i].start = PAGE_ALIGN_ROUND_UP(available_memory[i].start + phys_mmap_size * 4);
            break;
        }
        else if (available_memory[i].start + PAGE_SIZE + phys_mmap_size * 4 < available_memory[i].end)
        {
            phys_mmap = (unsigned long*)(available_memory[i].start + PAGE_SIZE);
                    
            available_memory[memblocks_count].start = available_memory[i].start;
            available_memory[memblocks_count].end = available_memory[i].start + PAGE_SIZE;
            memblocks_count++;
            
            available_memory[i].start = PAGE_ALIGN_ROUND_UP(available_memory[i].start + PAGE_SIZE + phys_mmap_size * 4);
            break;
        }
    }
    
    /* alle seiten als belegt markieren */
    phys_mark_page_range_as_used(0, upper_end / PAGE_SIZE);

    /* freie seiten als frei markieren */
    for(i = 0; i < memblocks_count; i++)
    {
        //kprintf("Mark as free: %x, %d Pages\n", available_memory[i].start, (available_memory[i].end - available_memory[i].start) / PAGE_SIZE);
        phys_mark_page_range_as_free(available_memory[i].start, (available_memory[i].end - available_memory[i].start) / PAGE_SIZE);
    }
}
