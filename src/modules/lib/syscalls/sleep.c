#include "syscall.h"

void yield()
{
    asm(
        "mov %0, %%eax;"
        "int $0x30;"
    : : "i" (SYSCALL_PM_SLEEP) : "eax");
}
