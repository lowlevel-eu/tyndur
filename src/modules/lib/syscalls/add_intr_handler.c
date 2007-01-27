#include "syscall.h"
#include "types.h"

void add_intr_handler(dword intr)
{
    asm(
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
    : : "i" (SYSCALL_ADD_INTERRUPT_HANDLER), "r" (intr));
}
