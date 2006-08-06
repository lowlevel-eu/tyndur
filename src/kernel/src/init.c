/* Kernel-Initialisierung erfolgt hier,
 die Funktion init wird von header.asm aufgerufen */

#include "gdt.h"
#include "intr.h"
#include "kprintf.h"
#include "ports.h"
#include "multiboot.h"
#include "string.h"
#include "tss.h"

struct multiboot_info multiboot_info;

extern void init_paging(vaddr_t mmap_addr, dword mmap_length);
extern void init_scheduler(void);

void foo()
{
    int i;

    while(1)
    {
        kprintf("a");
        for(i = 0; i < 10000; i++) { __asm__ __volatile__("nop"); }
    }
}

void bar()
{
    int i;
    while(1)
    {
        kprintf("b");
        for(i = 0; i < 10000; i++) { __asm__ __volatile__("nop"); }
    }
}

void baz()
{
    int i;
    while(1)
    {
        kprintf("c");
        for(i = 0; i < 10000; i++) { __asm__ __volatile__("nop"); }
    }
}

void init(int multiboot_magic, struct multiboot_info *boot_info)
{
    memcpy(&multiboot_info, boot_info, sizeof(struct multiboot_info));

    init_paging(multiboot_info.mi_mmap_addr, multiboot_info.mi_mmap_length);
    init_gdt();

    clrscr();

    init_idt();
    init_scheduler();

    create_task(foo);
    create_task(bar);
    create_task(baz);

    enable_interrupts();

    while(1)
    {
        __asm__ __volatile__("hlt");
    }
}

void panic(char * message, ...)
{
    int * args = ((int*)&message) + 1;
    disable_interrupts();
    kprintf("PANIC: ");
    kaprintf(message, &args);
    for(;;);
}
