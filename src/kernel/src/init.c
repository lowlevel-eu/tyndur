/* Kernel-Initialisierung erfolgt hier,
 die Funktion init wird von header.asm aufgerufen */

#include "grub.h"
#include "kprintf.h"
#include "ports.h"

void asm_test_handler();
asm("asm_test_handler:\n"
    "call test_handler\n"
    "iret");

void test_handler() {
  kprintf("  Jippie - ein Interrupt!\n");
}

void init(int multiboot_magic, multiboot_info_t *boot_info)
{
    clrscr();
   
    kprintf("Initialising LOST\n");

    kprintf("Available memory: %dK lower / %dK upper memory\n", boot_info->mem_lower,  boot_info->mem_upper);

    kprintf("Initialisiere Deskriptortabellen...");
    init_gdt();
    init_idt();
    
    kprintf(" ok\n");
    

    // Testweise Interrupt aufrufen
    set_intr(0x21, 0x01, &asm_test_handler);
    kprintf("\nTest-Interrupt aufrufen...\n");
    asm("int $0x21");

    // kprintf testen
    kprintf("\n%x\n", 0xabcdef12);
    kprintf("%llx\n", 0x1234567890abcdefLL);

    kprintf("\n%d\n", 0xabcdef12);
    kprintf("%lld\n", 1234567891234567LL);

    while(1);
}
