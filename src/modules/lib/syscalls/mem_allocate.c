#include "syscall.h"
#include "types.h"

void* mem_allocate(dword size, dword flags) {
    void* ptr;

    asm(
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x8, %%esp;"
    : "=a" (ptr) : "i" (SYSCALL_MEM_ALLOCATE), "r" (size), "r" (flags));

    return ptr;
}
