/* Kernel-Initialisierung erfolgt hier,
 die Funktion init wird von header.asm aufgerufen */

#include "gdt.h"
#include "intr.h"
#include "kprintf.h"
#include "ports.h"
#include "multiboot.h"
#include "string.h"
#include "task_switcher.h"
#include "tss.h"

void asm_test_handler();
asm("asm_test_handler:\n"
    "call test_handler\n"
    "iret");

void test_handler() {
  kprintf("  Jippie - ein Interrupt!\n");
}

void timer_irq(int irq)
{
    kprintf("tick\n");
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
    set_intr(0x30, 0x08, &asm_test_handler, 3, IDT_INTERRUPT_GATE);
    kprintf("\nTest-Interrupt aufrufen...\n");
    asm("int $0x30");

    request_irq(0, timer_irq);

    /* testweise in den ring 3 wechseln */
    tss.ss0 = SYS_DATA_SEL;
    tss.esp = 0x80000;

    __asm__("push $0x23\n" /* ss */
    "push $0x90000\n" /* esp */
    "push $0x3202\n" /* iopl=3 */
    "push $0x1b\n" /* cs */
    "push $usr_code\n" /* eip */
    "iret\n"); /* in den ring 3 wechseln */
}

void usr_code()
{
    __asm__("mov $0x23, %ax\n"
    "mov %ax, %ds\n"
    "mov %ax, %es\n"); /* ring 3 daten selektor laden */

    kprintf("Hallo aus dem User Mode!\n");

    kprintf("Interrupt aufrufen\n");
    asm("int $0x30");

    kprintf("fertig :)\n");
    for(;;);
}

void panic(char * message, ...)
{
    int * args = ((int*)&message) + 1;
    asm("cli");
    kprintf("PANIC: ");
    kaprintf(message, &args);
    for(;;);
}
