#include <stdint.h>
#include "syscall.h"

uint32_t create_shared_memory(uint32_t size)
{
    uint32_t id;

    asm(
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
    : "=a" (id) : "i" (SYSCALL_SHM_CREATE), "r" (size));

    return id;
}

void *open_shared_memory(uint32_t id)
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

ssize_t shared_memory_size(uint32_t id)
{
    int32_t size;

    asm(
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
    : "=a" (size) : "i" (SYSCALL_SHM_SIZE), "r" (id));

    return size;
}

void close_shared_memory(uint32_t id)
{
    asm(
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
    :: "i" (SYSCALL_SHM_DETACH), "r" (id));
}
