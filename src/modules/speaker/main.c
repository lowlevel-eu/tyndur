#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"
#include "ports.h"

void switch_speaker_on(void)
{
    outb(0x61,inb(0x61) | 3);
}

void switch_speaker_off(void)
{
    outb(0x61,inb(0x61) &~3);
}

void sound(unsigned frequenz)
{
    unsigned deling;
    deling = 1193180L/frequenz;
    outb(0x43,0xB6);
    outb(0x42,deling&0xFF);
    outb(0x42,deling >> 8);
    switch_speaker_on();
}

int main(int argc, char* argv[])
{
    request_ports(0x61,1);
    request_ports(0x42,2);
    puts("[SPEAKER] Module loaded.");

    return 0;
}
