#include <types.h>

void _start()
{
    while(TRUE) asm("int $0xFC");
}

