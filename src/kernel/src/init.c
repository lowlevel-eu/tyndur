/* Kernel-Initialisierung erfolgt hier,
 die Funktion init wird von header.asm aufgerufen */

#include "grub.h"
#include "kprintf.h"

void init(int multiboot_magic, multiboot_info_t *boot_info)
{
    kprintf("Initialising LOST\n");

    kprintf("Available memory: %dK lower / %dK upper memory\n", boot_info->mem_lower,  boot_info->mem_upper);

    while(1);
}
