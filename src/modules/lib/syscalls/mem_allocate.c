#include "types.h"

void* mem_allocate(dword size, dword flags) {
    void* ptr;

    asm(
        "pushl %0;"
        "pushl %1;"
        "mov $1, %%eax;"
        "int $0x30;"
        "add $0x8, %%esp;"
    : "=a" (ptr) : "r" (size), "r" (flags));

    return ptr;
}
