#include "syscall.h"
#include "types.h"

dma_mem_ptr_t mem_dma_allocate(dword size, dword flags)
{
    dma_mem_ptr_t ptr;

    asm(
        "pushl %4;"
        "pushl %3;"
        "mov %2, %%eax;"
        "int $0x30;"
        "add $0x8, %%esp;"
    : "=a" (ptr.virt), "=b" (ptr.phys) : "i" (SYSCALL_MEM_ALLOCATE), "r" (size), "r" (flags));

    return ptr;
}

void* mem_allocate(dword size, dword flags)
{
    dma_mem_ptr_t ptr = mem_dma_allocate(size, flags);
    return ptr.virt;
}

void mem_free(void* address, dword size)
{
    asm(
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
    : : "i" (SYSCALL_MEM_FREE), "r" (address));
}

