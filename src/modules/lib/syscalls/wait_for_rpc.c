#include "syscall.h"

void wait_for_rpc()
{
    asm(
        "mov %0, %%eax;"
        "int $0x30;"
    : : "i" (SYSCALL_PM_WAIT_FOR_RPC) : "eax");
}
