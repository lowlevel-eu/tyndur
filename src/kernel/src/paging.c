#include "bitops.h"
#include "kernel.h"
#include "multiboot.h"
#include "vmm.h"

/* Diese beiden werden in kernel.ld definiert. */
extern void kernel_phys_start();
extern void kernel_phys_end();

unsigned long * phys_mmap;
unsigned long phys_mmap_size;

unsigned long * page_directory;

void phys_mark_page_as_free(void * page)
{
    phys_mmap[(unsigned int)page / PAGE_SIZE / 32] |= 1 << (((unsigned int)page / PAGE_SIZE) & 31);
}

void phys_mark_page_as_used(void * page)
{
    phys_mmap[(unsigned int)page / PAGE_SIZE / 32] &= ~(1 << (((unsigned int)page / PAGE_SIZE) & 31));
}

void * find_free_page(unsigned long lower_limit)
{
    unsigned int i, j;
    void * page = 0;
    for(i = lower_limit / PAGE_SIZE / 32; i < phys_mmap_size; i++)
    {
        if(phys_mmap[i])
        {
            j = bit_scan_forward(phys_mmap[i]);
            /* assert(j <= 31); */
            page = (void*)((i * 32 + j) * PAGE_SIZE);
            phys_mark_page_as_used(page);
            return page;
        }
    }

    return 0;
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
            available_memory[memblocks_count].start = mmap->mm_base_addr;
            available_memory[memblocks_count].end = mmap->mm_base_addr + mmap->mm_length;

            memblocks_count++;
        }
    }

    /* In der Liste der physischen Speicherblöcke (im folgenden einfach
       Blöcke bzw. Block) einen Bereich, der von start und end angegeben wird,
       als nicht verfügbar definieren.

       Diese Funktion wird durch die Annahmen vereinfacht, dass GRUB den
       Anfang und das Ende eines Moduls immer in genau einen Speicherblock lädt.
       Außerdem dadurch, dass die Liste der verfügbaren Speicherblöcke nicht
       sortiert sein muss.
     */
    void carve(dword start, dword end)
    {
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

    /* einen ort für die tabelle mit den physischen seiten suchen */
    phys_mmap_size = upper_end / PAGE_SIZE / 32;
    for(i = 0; i < memblocks_count; i++)
    {
        if(available_memory[i].start + phys_mmap_size < available_memory[i].end)
        {
            phys_mmap = (unsigned long*)available_memory[i].start;
            available_memory[i].start += phys_mmap_size;
            break;
        }
    }

    /* alle seiten als belegt markieren */
    for(i = 0; i < phys_mmap_size; i++)
    {
        phys_mmap[i] = 0;
    }

    /* freie seiten als frei markieren */
    for(i = 0; i < memblocks_count; i++)
    {
        for(j = available_memory[i].start; j < available_memory[i].end; j += PAGE_SIZE)
        {
            phys_mark_page_as_free((void*)j);
        }
    }
}

void init_paging(vaddr_t mmap_addr, dword mmap_length)
{
    int i;

    init_phys_mem(mmap_addr, mmap_length);

    /* page directory initialisieren */
    page_directory = (unsigned long*)phys_alloc_page();
    for(i = 0; i < 1024; i++)
    {
        page_directory[i] = (i * PAGE_SIZE) | PTE_P | PTE_W | PTE_U | PTE_PS; /* alles 1:1 mappen. present, writeable, user, 4 mb pages */
    }
    page_directory[768] = PTE_P | PTE_W | PTE_U | PTE_PS; /* die unteren 4 mb werden nach 0xc0000000 gemappt */

    __asm__(
    "movl %%cr4, %%eax\n\t"
    "orl $0x00000010, %%eax\n\t"
    "movl %%eax, %%cr4\n\t" // PSE aktivieren

    "movl %0, %%cr3\n\t" // Page directory laden
    : : "r"(page_directory) : "eax");
}
