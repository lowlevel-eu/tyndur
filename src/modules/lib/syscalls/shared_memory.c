#include "syscall.h"
#include "types.h"

dword create_shared_memory(dword size)
{
    dword id;

    asm(
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x8, %%esp;"
    : "=a" (id) : "i" (SYSCALL_SHM_CREATE), "r" (size));

    return id;
}
void *open_shared_memory(dword id)
{
    vaddr_t addr;

    asm(
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
    : "=a" (addr) : "i" (SYSCALL_SHM_ATTACH), "r" (id));

    return addr;
}
void close_shared_memory(dword id)
{
    asm(
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
    :: "i" (SYSCALL_SHM_DETACH), "r" (id));
}
