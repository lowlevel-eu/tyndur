#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "init.h"

void irq_handler();

void _start() {
    puts("Keyboard");
    init_messaging();
    init_memory_manager();
    yield();
    
    init_service_register("keyboard");
    
    register_intr_handler(0x21, &irq_handler);
    while(1) yield();
}

void irq_handler() {
    dword scancode;

    puts("Keyboard IRQ.");
    asm volatile (
        "xor %%eax, %%eax;"
        "inb $0x60, %%al;"
    : "=a" (scancode));
}
