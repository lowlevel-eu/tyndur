#include "types.h"
#include "syscall.h"
#include "rpc.h"

void irq_handler();

void _start() {
    init_messaging();
    register_intr_handler(0x21, &irq_handler);

    while(1);
}

void irq_handler() {
    dword scancode;

    puts("Keyboard IRQ.");
    asm volatile (
        "xor %%eax, %%eax;"
        "inb $0x60, %%al;"
    : "=a" (scancode));
}
