/* Kernel-Initialisierung erfolgt hier,
 die Funktion init wird von header.asm aufgerufen */

#include "multiboot.h"
#include "kprintf.h"
#include "ports.h"
#include "string.h"
#include "task_switcher.h"

void asm_test_handler();
asm("asm_test_handler:\n"
    "call test_handler\n"
    "iret");

void test_handler() {
  kprintf("  Jippie - ein Interrupt!\n");
}

struct multiboot_info multiboot_info;


extern void init_paging(vaddr_t mmap_addr, dword mmap_length);

extern void * phys_alloc_page();
extern void * phys_alloc_dma_page();
extern void phys_free_page(void * page);


void init(int multiboot_magic, struct multiboot_info *boot_info)
{
    memcpy(&multiboot_info, boot_info, sizeof(struct multiboot_info));

    init_paging(multiboot_info.mi_mmap_addr, multiboot_info.mi_mmap_length);
    init_gdt();

    clrscr();

    kprintf("Initialising LOST\n");

    kprintf("Available memory: %dK lower / %dK upper memory\n", boot_info->mi_mem_lower,  boot_info->mi_mem_upper);

    kprintf("Initialisiere Deskriptortabellen...");
    init_idt();
    kprintf(" ok\n");

    kprintf("Initialisiere Taskswitcher...");
    init_taskswitcher();
    kprintf(" ok\n");

    // Testweise Interrupt aufrufen
    set_intr(0x21, 0x08, &asm_test_handler);
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
