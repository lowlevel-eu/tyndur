#include "syscall.h"


void init_child_page (pid_t pid, void* dest, void* src, size_t size)
{
    asm(
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x10, %%esp;"
    : : "i" (SYSCALL_PM_INIT_PAGE), "r"(pid), "r" (dest), "r" (src), "r" (size));
}
