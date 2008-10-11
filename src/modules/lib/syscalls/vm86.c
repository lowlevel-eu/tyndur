
#include "syscall.h"

bool vm86_int(vm86_regs_t *regs, dword *shm)
{
    dword result;
    asm(
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x8, %%esp;"
    : "=a"(result): "i" (SYSCALL_VM86), "r"(regs), "r"(shm) );
    return (bool)result;
}

