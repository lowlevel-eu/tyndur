#include "syscall.h"

memory_info_t memory_info()
{
    memory_info_t result;

    asm(
        "mov %2, %%eax;"
        "int $0x30;"
    : "=a" (result.total), "=d" (result.free): "i" (SYSCALL_MEM_INFO));

    return result;
}
