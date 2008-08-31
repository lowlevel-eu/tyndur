#include "syscall.h"

void set_rpc_handler(void (*rpc_handler)(void))
{
    asm(
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
    : : "i" (SYSCALL_SET_RPC_HANDLER), "r" (rpc_handler));
}
