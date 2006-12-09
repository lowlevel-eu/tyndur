#include "types.h"
#include "syscall.h"

extern void rpc_handler();

void _start() {
    set_rpc_handler(&rpc_handler);
    add_intr_handler(0x21);

    while(1);
}

void irq_handler() {
    dword scancode;

    puts("Keyboard IRQ.");
    asm(
        "xor %%eax, %%eax;"
        "inb $0x60, %%al;"
    : "=a" (scancode));
    asm(
        "outb %%al, %%dx"
	: : "a" (0x20), "d" (0x20));
}
