#include "syscall.h"

void p()
{
    asm(
        "mov %0, %%eax;"
        "int $0x30;"
    : : "i" (SYSCALL_PM_P) : "eax");
}

void v()
{
    asm(
        "mov %0, %%eax;"
        "push $0;"
        "int $0x30;"
        "add $4, %%esp;"
    : : "i" (SYSCALL_PM_V) : "eax");
}
