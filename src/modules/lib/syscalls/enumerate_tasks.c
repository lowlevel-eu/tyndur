#include "syscall.h"

task_info_t* enumerate_tasks()
{
    dword addr;
    asm(
        "mov %1, %%eax;"
        "int $0x30;"
    : "=a" (addr) : "i" (SYSCALL_PM_ENUMERATE_TASKS));

    return (task_info_t*) addr;
}
