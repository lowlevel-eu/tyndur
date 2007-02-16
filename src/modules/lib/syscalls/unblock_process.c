#include "syscall.h"


void unblock_process(pid_t pid)
{
    asm(
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
        : : "i" (SYSCALL_PM_V), "r" (pid));
}
