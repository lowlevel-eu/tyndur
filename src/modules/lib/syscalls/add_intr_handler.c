#include "types.h"

void add_intr_handler(dword intr)
{
    asm(
        "pushl %0;"
        "mov $52, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
    : : "r" (intr));
}
