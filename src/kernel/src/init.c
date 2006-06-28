/* Kernel-Initialisierung erfolgt hier,
 die Funktion init wird von header.asm aufgerufen */

#include "multiboot.h"
#include "kprintf.h"
#include "ports.h"
#include "string.h"

void asm_test_handler();
asm("asm_test_handler:\n"
    "call test_handler\n"
    "iret");

void test_handler() {
  kprintf("  Jippie - ein Interrupt!\n");
}

struct multiboot_info multiboot_info;


extern void init_paging(vaddr_t mmap_addr, dword mmap_length, unsigned int delta_load);

extern void * phys_alloc_page();
extern void * phys_alloc_dma_page();
extern void phys_free_page(void * page);


void init(int multiboot_magic, struct multiboot_info *boot_info, unsigned int delta_load)
{
    memcpy(&multiboot_info, (char*)boot_info - delta_load, sizeof(struct multiboot_info));

    init_paging((char*)multiboot_info.mi_mmap_addr - delta_load, multiboot_info.mi_mmap_length, delta_load);
    init_gdt();

    clrscr();

    kprintf("Initialising LOST\n");

    kprintf("Available memory: %dK lower / %dK upper memory\n", boot_info->mi_mem_lower,  boot_info->mi_mem_upper);

    kprintf("Initialisiere Deskriptortabellen...");
    init_idt();

    kprintf(" ok\n");

    // Testweise Interrupt aufrufen
    set_intr(0x21, 0x01, &asm_test_handler);
    kprintf("\nTest-Interrupt aufrufen...\n");
    asm("int $0x21");
    
    while(1);
}

void panic(char * message)
{
    kprintf("PANIC: %s\n", message);
    asm("cli");
    for(;;);
}
