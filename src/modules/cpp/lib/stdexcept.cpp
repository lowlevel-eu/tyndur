extern "C"
{
    #include "syscall.h"
    void __gxx_personality_v0();
}

void __gxx_personality_v0()
{
    puts("STD-Exception!");
    while(true) asm("nop");
}

