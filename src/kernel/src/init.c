/* Kernel-Initialisierung erfolgt hier,
 die Funktion init wird von header.asm aufgerufen */

#include "multiboot.h"
#include "kprintf.h"
#include "ports.h"

/*void asm_test_handler();
asm("asm_test_handler:\n"
    "call test_handler\n"
    "iret");

void test_handler() {
  kprintf("  Jippie - ein Interrupt!\n");
}*/

struct multiboot_info multiboot_info;

extern void init_paging();

extern void * phys_alloc_page();
extern void * phys_alloc_dma_page();
extern void phys_free_page(void * page);


void init(int multiboot_magic, struct multiboot_info *boot_info)
{
    multiboot_info = *boot_info;
    void *a,*b,*c,*d;

    init_paging();
    init_gdt();

    kprintf("%p\n", phys_alloc_page());
    kprintf("%p\n", phys_alloc_page());
    kprintf("%p\n", phys_alloc_page());
    kprintf("%p\n", phys_alloc_page());
    kprintf("%p\n", phys_alloc_page());
    kprintf("%p\n", phys_alloc_page());
    kprintf("%p\n", phys_alloc_page());
    kprintf("%p\n", phys_alloc_page());
    kprintf("%p\n", a=phys_alloc_dma_page());
    kprintf("%p\n", b=phys_alloc_dma_page());
    kprintf("%p\n", c=phys_alloc_dma_page());
    kprintf("%p\n", d=phys_alloc_dma_page());
    phys_free_page(c);
    phys_free_page(a);
    //phys_free_page(b);
    phys_free_page(d);
    kprintf("%p\n", phys_alloc_dma_page());
    kprintf("%p\n", phys_alloc_dma_page());
    kprintf("%p\n", phys_alloc_dma_page());
    kprintf("%p\n", phys_alloc_dma_page());
    kprintf("%p\n", phys_alloc_dma_page());

    /*clrscr();

    kprintf("Initialising LOST\n");

    kprintf("Available memory: %dK lower / %dK upper memory\n", boot_info->mi_mem_lower,  boot_info->mi_mem_upper);

    kprintf("Initialisiere Deskriptortabellen...");
    init_gdt();
    init_idt();

    kprintf(" ok\n");


    // Testweise Interrupt aufrufen
    set_intr(0x21, 0x01, &asm_test_handler);
    kprintf("\nTest-Interrupt aufrufen...\n");
    asm("int $0x21");

    while(1);*/
}

void panic(char * message)
{
    kprintf("PANIC: %s\n", message);
    asm("cli");
    for(;;);
}
