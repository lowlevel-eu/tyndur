#include "syscall.h"

int syscall_putsn(unsigned int n, const char* s) {
    
    asm("push %1;"
        "push %2;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x8, %%esp" 
    : : "i" (SYSCALL_PUTSN) , "r" (n) , "r" (s));

    return 1;
}


