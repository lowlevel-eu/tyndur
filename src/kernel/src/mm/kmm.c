#include "types.h"
#include "string.h"
#include "vmm.h"

// Der Teil des Page Directorys, der den Kernelraum mappt und für alle Prozesse gleich ist
//dword kernel_page_tables[NUM_KERNEL_PAGE_TABLES];

#define kernel_page_tables ((dword*)(kernel_page_directory + (KERNEL_BASE >> PGDIR_SHIFT)))
extern page_directory_t kernel_page_directory;

static page_table_t get_kernel_page_table_addr(dword page_table)
{
    if(page_table < KERNEL_BASE)
    {
        /* TODO: Ein panic mit (einem noch zu schreibenden) BUG()-Makro könnte hier angebrachter sein */
        panic("page_table(%x) < KERNEL_BASE", page_table);
    }

    return (page_table_t)(KERNEL_PAGE_TABLES_VADDR + (((page_table - KERNEL_BASE) >> PGDIR_SHIFT) << PAGE_SHIFT));
}

static void map_kernel_page_table(int pgdir_index, dword addr)
{
    page_table_t page_table = get_kernel_page_table_addr(KERNEL_PAGE_TABLES_VADDR);

    page_table[pgdir_index] = addr | PTE_W | PTE_P;
}
/*
void kernel_mem_init()
{
    page_table[pgdir_index] = phys_alloc_page() | PTE_W | PTE_P;
}*/

void * alloc_kernel_page()
{
    int i, j;
    page_table_t page_table;

    kprintf("kernel_page_tables %x\n", kernel_page_tables);
    
   // for(;;);
    
    // Eine Page Table suchen, die frei ist.
    for(i = 0; i < NUM_KERNEL_PAGE_TABLES; i++)
    {
        if(kernel_page_tables[i] & PTE_P)
        {
            page_table = (page_table_t)(KERNEL_PAGE_TABLES_VADDR + (i << PAGE_SHIFT)); // get_kernel_page_table_addr(kernel_page_tables[i] & ~PAGE_MASK);
            
    kprintf("kernel_page_tables[%d] %x\npage_table %x", i, kernel_page_tables[i], page_table);
            for(;;);
            
            for(j = 0; j < PAGE_TABLE_LENGTH; j++)
            {
                if(!(page_table[j] & PTE_P))
                {


    kprintf("alloc_kernel_page()\n", kernel_page_tables);
            for(;;);
    
                    // Unsere neue Page in die Page Table eintragen
                    page_table[j] = phys_alloc_page() | PTE_W | PTE_P;
                    return (void*)(KERNEL_BASE + (i << PGDIR_SHIFT) + (j << PAGE_SHIFT));
                }
            }
        }
    }

    // Eine freien Eintrag im Page Directory für eine neue Page Table suchen.
    for(i = 0; i < NUM_KERNEL_PAGE_TABLES; i++)
    {
        if(!(kernel_page_tables[i] & PTE_P))
        {
            kernel_page_tables[i] = phys_alloc_page() | PTE_W | PTE_P;
            
            map_kernel_page_table(i, kernel_page_tables[i]);
            //page_table = get_kernel_page_table_addr(kernel_page_tables[i] & ~PAGE_MASK);
            page_table = (page_table_t)(KERNEL_PAGE_TABLES_VADDR + (i << PAGE_SHIFT)); 

            // Page Table leeren
            memset(page_table, PAGE_SIZE, 0);

            // Unsere neue Page an den Anfang eintragen
            page_table[0] = phys_alloc_page() | PTE_W | PTE_P;

            return (void*)(KERNEL_BASE + (i << PGDIR_SHIFT));
        }
    }
            
    panic("Der virtuelle Adressraum > 3GB ist voll.");
}

void free_kernel_page(vaddr_t page)
{
    dword addr = (dword)page;
    
    if(addr < KERNEL_BASE)
    {
        /* TODO: Ein panic mit (einem noch zu schreibenden) BUG()-Makro könnte hier angebrachter sein */
        panic("Es wurde versucht eine Seite außerhalb des Kernelraums freizugeben.");
    }
    
   /* addr - KERNEL_BASE
    
    kernelpage?*/
}
