#include "kernel.h"
#include "multiboot.h"

unsigned long * phys_mmap;
unsigned long phys_mmap_size;

unsigned long * page_directory;

void phys_mark_page_as_free(void * page)
{
    phys_mmap[(unsigned int)page / 4096 / 32] |= 1 << (((unsigned int)page / 4096) & 31);
}

void phys_mark_page_as_used(void * page)
{
    phys_mmap[(unsigned int)page / 4096 / 32] &= ~(1 << (((unsigned int)page / 4096) & 31));
}

/* Voraussetzung: Mindestens ein Bit von i muss gesetzt sein. */
unsigned int bit_scan_forward(unsigned int i)
{
    unsigned int j;
#if 0
    for(j = 0; j < 32; j++)
    {
        if(i & (1 << j))
        {
            return j;
        }
    }
#else
    __asm__("bsfl %1, %0\n\t" : "=r"(j) : "g"(i));
    return j;
#endif
}

void * find_free_page(unsigned long lower_limit)
{
    unsigned int i, j;
    void * page = 0;
    for(i = lower_limit / 4096 / 32; i < phys_mmap_size; i++)
    {
        if(phys_mmap[i])
        {
            j = bit_scan_forward(phys_mmap[i]);
            /* assert(j <= 31); */
            page = (void*)((i * 32 + j) * 4096);
            phys_mark_page_as_used(page);
            return page;
        }
    }

    panic("Kein freier Speicher.");
}

void phys_free_page(void * page)
{
    phys_mark_page_as_free(page);
}

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

void init_paging(vaddr_t mmap_addr, dword mmap_length, unsigned int delta_load)
{
    /* freien physischen speicher ermitteln */
    struct multiboot_mmap * mmap;
    struct {
        dword start;
        dword end;
    } available_memory[16];
    int i = 0, j, k;
    dword upper_end = 0;
    unsigned int phys_mmap_phys_addr, page_directory_phys_addr;

    for(mmap = (struct multiboot_mmap*)mmap_addr;
        mmap < (struct multiboot_mmap*)(mmap_addr + mmap_length);
        mmap = (struct multiboot_mmap*)((char*)mmap + mmap->mm_size + 4))
    {
        if(mmap->mm_type == 1)
        {
            available_memory[i].start = mmap->mm_base_addr;
            available_memory[i].end = mmap->mm_base_addr + mmap->mm_length;

            /* die ersten 4 mb sind reserviert. todo: das hier verbessern. */
            if(available_memory[i].start <= 0x400000)
            {
                available_memory[i].start = 0x400000;
            }
            if(available_memory[i].end <= 0x400000)
            {
                available_memory[i].end = 0x400000;
            }
            if(available_memory[i].start < available_memory[i].end)
            {
                i++;
            }
        }
    }

    /* die obere grenze des physischen speichers ermitteln */
    for(j = 0; j < i; j++)
    {
        if(available_memory[j].end > upper_end)
        {
            upper_end = available_memory[j].end;
        }
    }


    /* einen ort für die tabelle mit den physischen seiten suchen */
    phys_mmap = 0;
    for(j = 0; j < i; j++)
    {
        if(available_memory[j].start + upper_end / 4096 / 8 < available_memory[j].end)
        {
            phys_mmap_phys_addr = available_memory[j].start - upper_end / 4096 / 8;
            phys_mmap = (unsigned long*)(phys_mmap_phys_addr - delta_load);
            phys_mmap_size = upper_end / 4096 / 32;
            break;
        }
    }

    if(!phys_mmap)
    {
        panic("Nicht genug Speicher. :(");
    }

    /* alle seiten als belegt markieren */
    for(j = 0; j < phys_mmap_size; j++)
    {
        phys_mmap[j] = 0;
    }

    /* freie seiten als frei markieren */
    for(k = 0; k < i; k++)
    {
        for(j = available_memory[k].start; j < available_memory[k].end; j += 4096)
        {
            phys_mark_page_as_free((void*)j);
        }
    }

    /* page directory initialisieren */
    page_directory_phys_addr = (unsigned long)phys_alloc_page();
    page_directory = (unsigned long*)(page_directory_phys_addr - delta_load);
    for(i = 0; i < 1024; i++)
    {
        page_directory[i] = (i * 4096) | 0x83; /* alles 1:1 mappen */
    }
    page_directory[768] = 0x83; /* nur die unteren 4 mb werden nach 0xc0000000 gemappt */
    page_directory[0] = (768 * 4096) | 0x83; /* nur die unteren 4 mb werden nach 0xc0000000 gemappt */

    __asm__("movl %0, %%cr3\n\t" // Page directory laden

    "movl %%cr4, %%eax\n\t"
    "orl $0x00000010, %%eax\n\t"
    "movl %%eax, %%cr4\n\t" // PSE aktivieren

    "movl %%cr0, %%eax\n\t"
    "orl $0x80000000, %%eax\n\t"
    "movl %%eax, %%cr0\n\t" // Paging aktivieren
    : : "r"(page_directory_phys_addr));

    page_directory = (unsigned long*)page_directory_phys_addr;
    phys_mmap = (unsigned long*)phys_mmap_phys_addr;
}
