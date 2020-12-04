/*
 * Copyright (c) 2006-2007 The tyndur Project. All rights reserved.
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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "kernel.h"
#include "multiboot.h"
#include "mm.h"
#include "lock.h"

#define min(x,y) (((x) < (y)) ? (x) : (y))

/* Diese beiden werden in kernel.ld definiert. */
extern void kernel_phys_start(void);
extern void kernel_phys_end(void);

/** Bitmap aller physischen Seiten. Gesetztes Bit steht für freie Seite */
#define PMM_BITS_PER_ELEMENT (8 * sizeof(uint_fast32_t))
#define PMM_NUM_DMA_ELEMENTS ((16 * 1024 * 1024) / sizeof(uint_fast32_t))

static uint_fast32_t* pmm_bitmap;

/** Lock fuer die physikalische Speicherverwaltung */
static lock_t pmm_lock = 0;

/** 
 * Größe von pmm_bitmap in Elementen (= 1/8*sizeof(uint_fast32_t) der Anzahl
 * der physischen Seiten)
 *
 * @see pmm_bitmap
 */
static size_t pmm_bitmap_length;

/**
 * Gibt die Startadresse der Bitmap zurück
 */
void* pmm_get_bitmap_start()
{
    return pmm_bitmap;
}

/**
 * Setzt die Startadresse der Bitmap. Diese Funktion ist einzig und allein dazu
 * bestimmt, den Übergang von physischen zu virtuellen Adressen beim
 * Einschalten von Paging bewerkstelligen zu können.
 */
void pmm_set_bitmap_start(void* bitmap_start)
{
    pmm_bitmap = bitmap_start;
}

/**
 * Gibt die Göße der Bitmap in Bytes zurück
 */
size_t pmm_get_bitmap_size()
{
    return pmm_bitmap_length * PMM_BITS_PER_ELEMENT / 8;
}

/**
 * Gibt die Anzahl der freien Seiten zurueck
 *
 * @return Anzahl freier Seiten
 */
size_t pmm_count_free()
{
    size_t free_pages = 0;
    size_t i, j;
    
    for (i = 0; i < pmm_bitmap_length; i++) {
        for (j = 0; j < PMM_BITS_PER_ELEMENT; j++) {
            if (pmm_bitmap[i] & (1 << j)) {
                free_pages++;
            }
        }
    }

    return free_pages;
}

/**
 * Gibt die Anzahl der Pages zurueck
 */
size_t pmm_count_pages()
{
    return 8 * pmm_get_bitmap_size();
}

/**
 * Markiert eine Page als frei/unbenutzt.
 *
 * @param page Zeiger auf den Anfang der Page, die als frei markiert werden 
 * soll.
 */

static inline void phys_mark_page_as_free(paddr_t page)
{
    //lock(&pmm_lock);
    size_t bitmap_index = (size_t) page / PAGE_SIZE / PMM_BITS_PER_ELEMENT;
    pmm_bitmap[bitmap_index] |= 1 << (((size_t) page / PAGE_SIZE) & 
        (PMM_BITS_PER_ELEMENT - 1));

    //unlock(&pmm_lock);
}

/**
 * Markiert num Pages als frei/unbenutzt.
 *
 * @param page Zeiger auf den Anfang der ersten Page.
 * @param num Anzahl der Pages die als frei markiert werden sollen.
 */

static inline void phys_mark_page_range_as_free(paddr_t page, size_t num)
{
    size_t i;

    for(i = 0; i < num; i++) {
        phys_mark_page_as_free(page + i * PAGE_SIZE);
    }
}

/**
 * Markiert eine Page als benutzt.
 *
 * @param page Zeiger auf den Anfang der Page, die als benutzt markiert werden 
 * soll.
 */

static inline void phys_mark_page_as_used(paddr_t page)
{
    //lock(&pmm_lock);
    size_t bitmap_index = (size_t) page / PAGE_SIZE / PMM_BITS_PER_ELEMENT;
    pmm_bitmap[bitmap_index] &= ~(1 << (((size_t) page / PAGE_SIZE) & 
        (PMM_BITS_PER_ELEMENT - 1)));
    //unlock(&pmm_lock);
}

/**
 * Markiert num Pages als benutzt.
 *
 * @param page Zeiger auf den Anfang der ersten Page.
 * @param num Anzahl der Pages die als benutzt markiert werden sollen.
 */

static inline void phys_mark_page_range_as_used(paddr_t page, size_t num)
{
    size_t i;

    for(i = 0; i < num; i++) {
        phys_mark_page_as_used(page + i * PAGE_SIZE);
    }
}

/**
 * Sucht num freie Pages und gibt einen Zeiger auf den Anfang der ersten Page 
 * zurück.
 *
 * @param lower_index Index der niedrigsten erlaubten Page
 * @param upper_index Index der höchsten erlaubten Page
 * @param num Anzahl der Pages.
 *
 * @return Zeiger auf den Anfang der ersten Page.
 * @return Im Erfolgsfall wird ein Zeiger auf den Anfang der ersten 
 *      Page zurückgegeben (Bei Erfolg ist der Rückgabewert immer
 *      durch PAGE_SIZE teilbar). Im Fehlerfall wird 1 zurückgegeben.
 */

/* TODO: unbedingt testen */
static paddr_t find_free_page_range
    (size_t lower_index, size_t upper_index, size_t num)
{
    size_t i, j;
    size_t found = 0;
    paddr_t page = 0;

    for(i = lower_index; i < upper_index; i++)
    {
        if(pmm_bitmap[i] == 0) {
            found = 0;
            continue;
        }

        if(pmm_bitmap[i] == ~0x0)
        {
            if(found == 0)
            {
                page = i * PMM_BITS_PER_ELEMENT * PAGE_SIZE;
            }
            found += PMM_BITS_PER_ELEMENT;
        }
        else
        {
            for(j = 0; j < PMM_BITS_PER_ELEMENT; j++)
            {
                if(pmm_bitmap[i] & (1 << j))
                {
                    if(found == 0) {
                        page = (i * PMM_BITS_PER_ELEMENT + j) * PAGE_SIZE;
                    }
                    found++;

                    if(found > num) {
                        break;
                    }
                }
                else
                {
                    found = 0;
                }
            }
        }

        if(found > num) {
            return page;
        }
    }

    return 1;
}


/**
 * Reserviert num Pages.
 *
 * @param lower_limit Niedrigste erlaubte Adresse (Muß auf 32-Bit-Systemen
 * auf einer 128K-Grenze liegen. Wenn nicht, wird gerundet)
 *
 * @param upper_limit Höchste erlaubte Adresse (Muß auf 32-Bit-Systemen
 * auf einer 128K-Grenze liegen. Wenn nicht, wird gerundet)
 *
 * @param num Anzahl der zu reservierenden Seiten
 *
 * @return Zeiger auf den Anfang der ersten Page.
 */
paddr_t pmm_alloc_limits(paddr_t lower_limit, paddr_t upper_limit, size_t num)
{
    lock(&pmm_lock);
    paddr_t page;
    size_t lower_index = (size_t) lower_limit / PMM_BITS_PER_ELEMENT;
    size_t upper_index = (size_t) upper_limit / PMM_BITS_PER_ELEMENT;

    if (upper_index > pmm_bitmap_length) {
        upper_index = pmm_bitmap_length;
    }

    if ((upper_index > PMM_NUM_DMA_ELEMENTS) && 
        (lower_index < PMM_NUM_DMA_ELEMENTS))
    {
        page = find_free_page_range(lower_index, upper_index, num);
    }
    else
    {
        page = find_free_page_range(PMM_NUM_DMA_ELEMENTS, upper_index, num);
    
        if((size_t) page & (PAGE_SIZE - 1)) {
            page = find_free_page_range(
                lower_index, 
                min(PMM_NUM_DMA_ELEMENTS + num, upper_index), 
                num
            );
        }
    }
    
    if((size_t) page & (PAGE_SIZE - 1)) {
        unlock(&pmm_lock);
        panic("Kein freier Speicher mehr da.");
    } else {
        phys_mark_page_range_as_used(page, num);
        unlock(&pmm_lock);
        return page;
    }
}

/**
 * Reserviert num Pages, die beliebig im physischen Speicher liegen können.
 *
 * @see pmm_alloc_limits
 */
paddr_t pmm_alloc(size_t num)
{
    paddr_t result = pmm_alloc_limits(
        0,
        pmm_bitmap_length * PMM_BITS_PER_ELEMENT * PAGE_SIZE,
        num
    );

    return result;
}


/**
 * Gibt physisch zusammenhängende Speicherseiten frei
 *
 * @param start Zeiger auf das erste Byte der ersten freizugebenden Seite
 * @param count Anzahl der freizugebenden Seiten
 */
void pmm_free(paddr_t start, size_t count)
{
    // TODO Prüfen, ob die Seiten vorher auch besetzt waren, sonst Panic
    phys_mark_page_range_as_free(start, count);
}


struct mem_block {
    paddr_t start;
    paddr_t end;
};

/**
 * Wird benutzt, um nacheinander alle vom BIOS aus freien Speicherbereiche
 * aufzuzaehlen. Dies bedeutet nicht, dass die zurueckgegebenen
 * Speicherbereiche frei benutzt werden koennen. Um festzustellen, ob nicht
 * tyndur Bereiche benoetigt (z.B. fuer Kernel oder Multiboot-Strukturen), muss
 * zusaetzlich get_reserved_block() benutzt werden.
 *
 * @param multiboot_info Pointer auf die Multiboot-Info
 * @param i Index des freien Speicherbereichs
 *
 * @return Beschreibung des freien Speicherblocks. Wenn i groesser als die
 * Anzahl der freien Blocks ist, wird ein Block mit start > end
 * zurueckgegeben.
 */
static struct mem_block get_free_block
    (struct multiboot_info* multiboot_info, size_t i)
{
    struct mem_block result;

    // Überprüfung der BIOS-Memory-Map
    struct multiboot_mmap* mmap;
    uintptr_t mmap_addr = multiboot_info->mi_mmap_addr;
    uintptr_t mmap_end = mmap_addr + multiboot_info->mi_mmap_length;

    for(mmap = (struct multiboot_mmap*) (mmap_addr);
        mmap < (struct multiboot_mmap*) (mmap_end);
        mmap++)
    {
        uint64_t start = mmap->mm_base_addr;
        uint64_t end   = mmap->mm_base_addr + mmap->mm_length - 1;

        if (start > UINTPTR_MAX) {
            continue;
        }
        if (end > UINTPTR_MAX) {
            end = UINTPTR_MAX;
        }

        // Typ 1 steht für freien Speicher
        if (mmap->mm_type == 1) {
            if (i-- == 0) {
                result.start    = (paddr_t)start;
                result.end      = (paddr_t)end;
                return result;
            }
        }
    }

    result.start = (paddr_t) 0xFFFFFFFF;
    result.end   = (paddr_t) 0;

    return result;
}

/**
 * Wird benutzt, um nacheinander alle belegten Speicherbereiche aufzuzaehlen.
 * Dies sind vor allem Kernel, Module und Multiboot-Strukturen. Vom BIOS als
 * reserviert gekennzeichnete Bereiche sind hierbei nicht enthalten. Um die
 * vom BIOS aus freien Bereiche zu erhalten, muss get_free_block() benutzt
 * werdem.
 *
 * @param multiboot_info Pointer auf die Multiboot-Info
 * @param i Index des belegten Speicherbereichs
 * @param bitmap Wenn true, wird auch der Platz der Speicherbitmap als
 * reserviert gekennzeichnet. Wenn die Speicherbitmap noch nicht platziert
 * wurde, muss dieser Parameter false sein.
 *
 * @return Beschreibung des reservierten Blocks. Wenn i groesser als die
 * Anzahl der reservierten Blocks ist, wird ein Block mit start > end
 * zurueckgegeben.
 */
static struct mem_block get_reserved_block
    (struct multiboot_info* multiboot_info, size_t i, bool bitmap)
{
    struct mem_block result;

    // Bitmap nur beruecksichtigen, wenn sie bereits zugewiesen ist
    if (bitmap && (i-- == 0)) {
        result.start = (paddr_t) pmm_bitmap;
        result.end   = (paddr_t)
            (pmm_bitmap + ((pmm_bitmap_length / 8) * PMM_BITS_PER_ELEMENT));
        return result;
    }

    // Der Kernel ist besetzt
    if (i-- == 0) {
        result.start    = (paddr_t) kernel_phys_start;
        result.end      = (paddr_t) kernel_phys_end;
        return result;
    }

    // Die Module auch.
    {
        uint32_t j;
        struct multiboot_module* multiboot_module;
        multiboot_module = (struct multiboot_module*) (uintptr_t) 
            multiboot_info->mi_mods_addr;

        for (j = 0; j < multiboot_info->mi_mods_count; j++) 
        {
            // Die Multiboot-Info zum Modul    
            if (i-- == 0) {
                result.start    = (paddr_t) multiboot_module;
                result.end      = (paddr_t) (multiboot_module + 1);
                return result;
            }

            // Das Modul selbst
            if (i-- == 0) {
                result.start    = (paddr_t) multiboot_module->start;
                result.end      = (paddr_t) multiboot_module->end;
                return result;
            }

            // Die Kommandozeile des Moduls 
            if (multiboot_module->cmdline) {
                if (i-- == 0) {
                    result.start = (paddr_t) multiboot_module->cmdline;
                    result.end   = (paddr_t) multiboot_module->cmdline +
                        strlen((char*) (uintptr_t) multiboot_module->cmdline);
                    return result;
                }
            }

            multiboot_module++;
        }
    }

    result.start = (paddr_t) 0xFFFFFFFF;
    result.end   = (paddr_t) 0;

    return result;
}

/**
 * Suche einen Platz für die Bitmap
 */
static paddr_t find_bitmap_mem
    (struct multiboot_info* multiboot_info, size_t size)
{
    if (multiboot_info->mi_flags & MULTIBOOT_INFO_HAS_MMAP) {
        size_t i, j;
        struct mem_block free, reserved;
        paddr_t bitmap = 0x0;

        i = 0;
        do
        {
            // Freien Speicherblock suchen und Bitmap an den Anfang legen
            free = get_free_block(multiboot_info, i++);
            if (free.start > free.end) {
                panic("Keinen Platz fuer die Speicherbitmap gefunden");
            }
            bitmap = free.start;

            // Probieren, ob der Speicherblock nicht doch besetzt ist und
            // in diesem Fall ein Stueck weitergehen und nochmal von vorne
            // durchprobieren
            j = 0;
            do
            {
                reserved = get_reserved_block(multiboot_info, j++, false);
                if (!((reserved.start > bitmap + size)
                    || (reserved.end <= bitmap)))
                {
                    j = 0;
                    bitmap = (paddr_t)
                        PAGE_ALIGN_ROUND_UP((uintptr_t) reserved.end);
                }
            }
            while ((bitmap <= free.end) && (reserved.start < reserved.end));

            // Wenn die Bitmap nach Beruecksichtigung aller besetzten Bereiche
            // immer noch im freien Bereich liegt, dann nehmen wir die Adresse
            if (bitmap <= free.end) {
                return bitmap;
            }
        }
        while(true);
    } else {
        return (paddr_t) 0x1000;
    }
}

/**
 * Bestimmt die benötigte Größe der Bitmap in Bytes
 */
static size_t required_bitmap_size(struct multiboot_info* multiboot_info)
{
    // Suche die höchste Adresse
    paddr_t upper_end = 0;
    if (multiboot_info->mi_flags & MULTIBOOT_INFO_HAS_MMAP) 
    {
        struct multiboot_mmap* mmap =
            (struct multiboot_mmap*) (uintptr_t) multiboot_info->mi_mmap_addr;

        size_t i = 0;
        for (i = 0; i < multiboot_info->mi_mmap_length / sizeof(*mmap); i++)
        {
            if (((uint32_t) (mmap[i].mm_base_addr + mmap[i].mm_length)
                > (uint32_t) (uintptr_t) upper_end) && (mmap[i].mm_type == 1))
            {
                upper_end = (paddr_t)(uintptr_t) 
                    mmap[i].mm_base_addr + mmap[i].mm_length;
            }
        }
    }
    else 
    {
        // Der hintere Cast verhindert Compiler-Warungen
        upper_end = (paddr_t) (1024 * (1024 + (uintptr_t) 
            multiboot_info->mi_mem_upper));
    }

    // Umrechnen auf benötigte Bytes für die Bitmap
    return (size_t) upper_end / PAGE_SIZE / 8;
}

/**
 * Initialisiert den physischen Speicher.
 *
 * @param mmap_addr Adresse der Memorymap.
 * @param mmap_length Länge der Memorymap in Bytes.
 * @param upper_mem Größe des upper_mem in Kilobyte, wie in der Multboot-Info
 *      übergeben.
 */
void pmm_init(struct multiboot_info* multiboot_info)
{
    // Finde einen Platz für die Bitmap
    size_t bitmap_size = required_bitmap_size(multiboot_info);
    pmm_bitmap = (void*) find_bitmap_mem(multiboot_info, bitmap_size);
    pmm_bitmap_length = 8 * bitmap_size / PMM_BITS_PER_ELEMENT;

    // Am Anfang ist alles besetzt
    memset(pmm_bitmap, 0x0, bitmap_size);

    // ...anschließend die BIOS-Memory-Map abarbeiten
    size_t i = 0;
    do
    {
        struct mem_block block = get_free_block(multiboot_info, i++);

        // Wenn der Start nach dem Ende liegt, ist der Block ungueltig
        // und wir sind fertig
        if (block.start > block.end) {
            break;
        }

        // Reservierten Block als frei markieren
        phys_mark_page_range_as_free
            (block.start, NUM_PAGES(block.end - block.start));
    }
    while(true);


    // ...und dann Kernel usw. wieder reservieren
    i = 0;
    do
    {
        struct mem_block block = get_reserved_block(multiboot_info, i++, true);

        // Wenn der Start nach dem Ende liegt, ist der Block ungueltig
        // und wir sind fertig
        if (block.start > block.end) {
            break;
        }

        // Reservierten Block als besetzt markieren
        phys_mark_page_range_as_used
            (block.start, NUM_PAGES(block.end - block.start));
    }
    while(true);
}
