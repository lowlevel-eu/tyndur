#include "syscall.h"

int syscall_puts(const char* s) {
    
    asm("push %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp" 
    : : "i" (SYSCALL_KPRINTF) , "r" (s));

    return 1;
}

int puts(const char* s) {
    syscall_puts(s);
    syscall_puts("\n");

    return 1;
}
