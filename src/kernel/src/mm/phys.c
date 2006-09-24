/*
 * Copyright (c) 2006 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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

/*
 * 2006-23-9: malu
 * Dokumentation hinzugefügt
 */

#include "bitops.h"
#include "kernel.h"
#include "multiboot.h"
#include "vmm.h"

/* Diese beiden werden in kernel.ld definiert. */
extern void kernel_phys_start();
extern void kernel_phys_end();

unsigned long * phys_mmap;
unsigned long phys_mmap_size;

/**
 * Markiert eine Page als frei/unbenutzt.
 *
 * @param page Zeiger auf den Anfang der Page die als frei markiert werden soll.
 */

void phys_mark_page_as_free(void * page)
{
    phys_mmap[(unsigned int)page / PAGE_SIZE / 32] |= 1 << (((unsigned int)page / PAGE_SIZE) & 31);
}

/**
 * Markiert num Pages als frei/unbenutzt.
 *Startp
 * @param page Zeiger auf den Anfang der ersten Page.
 * @param num Anzahl der Pages die als frei markiert werden sollen.
 */

void phys_mark_page_range_as_free(void * page, unsigned int num)
{
    int i;

    for(i = 0; i < num; i++)
    {
        phys_mark_page_as_free((void*)((char*)page + i * PAGE_SIZE));
    }
}

/**
 * Markiert eine Page als benutzt.
 *
 * @param page Zeiger auf den Anfang der Page die als benutzt markiert werden soll.
 */

void phys_mark_page_as_used(void * page)
{
    phys_mmap[(unsigned int)page / PAGE_SIZE / 32] &= ~(1 << (((unsigned int)page / PAGE_SIZE) & 31));
}

/**
 * Markiert num Pages als benutzt.
 *
 * @param page Zeiger auf den Anfang der ersten Page.
 * @param num Anzahl der Pages die als benutzt markiert werden sollen.
 */

void phys_mark_page_range_as_used(void * page, unsigned int num)
{
    int i;

    for(i = 0; i < num; i++)
    {
        phys_mark_page_as_used((void*)((char*)page + i * PAGE_SIZE));
    }
}

/**
 * Sucht eine freie Page und gibt einen Zeiger auf den Anfang zurück.
 *
 * @param lower_limit Mindestgröße der Page.
 *
 * @return Zeiger auf den Anfang der Page.
 */

void * find_free_page(unsigned long lower_limit)
{
    unsigned int i, j;
    void * page = 0;
    for(i = lower_limit / PAGE_SIZE / 32; i < phys_mmap_size; i++)
    {
        if(phys_mmap[i])
        {
            j = bit_scan_forward(phys_mmap[i]);
            page = (void*)((i * 32 + j) * PAGE_SIZE);
            return page;
        }
    }

    return 0;
}

/**
 * Sucht num freie Pages und gibt einen Zeiger auf den Anfang der ersten Page zurück.
 *
 * @param lower_limit Mindestgröße jeder Page.
 * @param num Anzahl der Pages.
 *
 * @return Zeiger auf den Anfang der ersten Page.
 */

/* TODO: unbedingt testen */
void * find_free_page_range(unsigned long lower_limit, unsigned int num)
{
    unsigned int i, j;
    unsigned int found = 0;
    void * page = 0;

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
                page = (void*)(i * 32 * PAGE_SIZE);
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
                        page = (void*)((i * 32 + j) * PAGE_SIZE);
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

    return 0;
}

/**
 * Markiert eine Page als frei/unbenutzt.
 *
 * @param page Zeiger auf den Anfang der Page die als frei markiert werden soll.
 */

void phys_free_page(void * page)
{
    phys_mark_page_as_free(page);
}

/**
 * Markiert num Pages als frei/unbenutzt.
 *Startp
 * @param page Zeiger auf den Anfang der ersten Page.
 * @param num Anzahl der Pages die als frei markiert werden sollen.
 */

void phys_free_page_range(void * page, unsigned int num)
{
    phys_mark_page_range_as_free(page, num);
}

/**
 * Reserviert eine DMA-Page und markiert sie gleichteitig als benutzt.
 *
 * @return Zeiger auf den Anfang der DMA-Page.
 */

void * phys_alloc_dma_page()
{
    void * page = find_free_page(0);
    if(!page)
    {
        panic("Keine freier Speicher mehr da.");
    }
    phys_mark_page_as_used(page);
    return page;
}

/**
 * Reserviert eine Page.
 *
 * @return Zeiger auf den Anfang der Page.
 */

void * phys_alloc_page()
{
    void * page = find_free_page(16 * 1024 * 1024);
    if(!page)
    {
        return phys_alloc_dma_page();
    }
    phys_mark_page_as_used(page);
    return page;
}

/**
 * Reserviert num DMA-Pages.
 *
 * @return Zeiger auf den Anfang der ersten Page.
 */

void * phys_alloc_dma_page_range(unsigned int num)
{
    void * page = find_free_page_range(0, num);
    if(!page)
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

void * phys_alloc_page_range(unsigned int num)
{
    void * page = find_free_page_range(16 * 1024 * 1024, num);
    if(!page)
    {
        return phys_alloc_dma_page_range(num);
    }
    phys_mark_page_range_as_used(page, num);
    return page;
}

/**
 * Initialisiert den physischen Speicher.
 *
 * @param mmap_addr Adresse der Memorymap.
 * @param mmap_length Länge der Memorymap.

void init_phys_mem(vaddr_t mmap_addr, dword mmap_length)
{
    /* freien physischen speicher ermitteln */
    struct multiboot_mmap * mmap;
    struct {
        dword start;
        dword end;
    } available_memory[16];
    int memblocks_count = 0;
    int i, j;
    dword upper_end = 0;

    /* Die von GRUB gelieferte memory map in unser Format umwandeln. */
    for(mmap = (struct multiboot_mmap*)mmap_addr;
        mmap < (struct multiboot_mmap*)(mmap_addr + mmap_length);
        mmap = (struct multiboot_mmap*)((char*)mmap + mmap->mm_size + 4))
    {
        if(mmap->mm_type == 1)
        {
            available_memory[memblocks_count].start = PAGE_ALIGN_ROUND_UP(mmap->mm_base_addr);
            available_memory[memblocks_count].end = PAGE_ALIGN_ROUND_DOWN(mmap->mm_base_addr + mmap->mm_length);

            memblocks_count++;
        }
    }

    /* In der Liste der physischen Speicherbl�ke (im folgenden einfach
       Bl�ke bzw. Block) einen Bereich, der von start und end angegeben wird,
       als nicht verfgbar definieren.

       Diese Funktion wird durch die Annahmen vereinfacht, dass GRUB den
       Anfang und das Ende eines Moduls immer in genau einen Speicherblock l�t.
       Au�rdem dadurch, dass die Liste der verfgbaren Speicherbl�ke nicht
       sortiert sein muss.
     */
    void carve(dword start, dword end)
    {
        start = PAGE_ALIGN_ROUND_UP(start);
        end = PAGE_ALIGN_ROUND_DOWN(end);
        
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
                    // available_memory[i].start bleibt unver�dert
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
               l�t, der kein verfgbarer Speicher ist. */
        }
    }

    carve((dword)kernel_phys_start, (dword)kernel_phys_end);
    /* XXX: foreach(modul in module) { carve(modul.start, module.end); } // pseudocode btw ;) */

    /* Die obere Grenze des physischen Speichers ermitteln. */
    for(i = 0; i < memblocks_count; i++)
    {
        if(available_memory[i].end > upper_end)
        {
            upper_end = available_memory[i].end;
        }
    }

    /* einen ort fr die tabelle mit den physischen seiten suchen */
    phys_mmap_size = upper_end / PAGE_SIZE / 32;
    for(i = 0; i < memblocks_count; i++)
    {
        if(available_memory[i].start + phys_mmap_size < available_memory[i].end)
        {
            phys_mmap = (unsigned long*)available_memory[i].start;
            available_memory[i].start = PAGE_ALIGN_ROUND_UP(available_memory[i].start + phys_mmap_size);
            break;
        }
    }

    /* alle seiten als belegt markieren */
    phys_mark_page_range_as_used((void*)0, upper_end / PAGE_SIZE);

    /* freie seiten als frei markieren */
    for(i = 0; i < memblocks_count; i++)
    {
        phys_mark_page_range_as_free((void*)available_memory[i].start, (available_memory[i].end - available_memory[i].start) / PAGE_SIZE);
    }
}
