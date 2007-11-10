#include "syscall.h"
#include "types.h"

dma_mem_ptr_t mem_dma_allocate(dword size, dword flags)
{
    dma_mem_ptr_t ptr;

    asm(
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0xc, %%esp;"
    : "=a" (ptr.virt) : "i" (SYSCALL_MEM_ALLOCATE), "r" (size), "r" (flags), "r" (&ptr.phys));

    return ptr;
}

void* mem_allocate(dword size, dword flags)
{
    dma_mem_ptr_t ptr = mem_dma_allocate(size, flags);
    return ptr.virt;
}

void *mem_allocate_physical(dword size, dword position, dword flags)
{
    dma_mem_ptr_t ptr;

    asm(
            "pushl %4;"
            "pushl %3;"
            "pushl %2;"
            "mov %1, %%eax;"
            "int $0x30;"
            "add $0xC, %%esp;"
    : "=a" (ptr.virt) : "i" (SYSCALL_MEM_ALLOCATE_PHYSICAL), "r" (size), "r" (position), "r" (flags));

    return ptr.virt;
}

bool mem_free(void* address, dword size)
{
    dword result;
    asm(
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x8, %%esp;"
    : "=a" (result) : "i" (SYSCALL_MEM_FREE), "r" (address), "r" (size));

    return result;
}

void mem_free_physical(void* address, dword size)
{
    asm(
            "pushl %2;"
            "pushl %1;"
            "mov %0, %%eax;"
            "int $0x30;"
            "add $0x8, %%esp;"
    : : "i" (SYSCALL_MEM_FREE_PHYSICAL), "r" (address), "r" (size));
}
