#include "types.h"
#include "syscall.h"

void putn(unsigned long long x, int radix, int pad, char padchar);

void _start() {
    char *hw = "Hello world!";
    int i;
    /*
    dword ptr;

    asm(
        "push %0;"
        "xor %%eax, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
        : : "r" (hw));
    putn(1337, 10, 10, '.');
    puts("\n");

    for (i = 0; i < 3; i++) {
        ptr = (dword) mem_allocate(0x1234, 0xf5);

        puts("\n[");
        putn(i, 10, 2, ' ');
        puts("] Reservierter Speicher an ");
        putn(ptr, 16, 10, '.');
    }
    */

    puts("ok\n\033[1;33mFuehre RPC durch...\033[0;37m\n");
    while (1)
    {
    for (i = 0; i < 5; i++) {
        if (i = 1)
        {
           puts("\nSende Test1");
           send_message(1, 512, 0, 8, "TEST___1");
        }
        if (i = 2)
        {
           puts("\nSende Test2");
           send_message(1, 512, 0, 8, "TEST___2");
        }
    }
    }
    /*
    asm(
        "pushl $0;"
        "pushl $0;"
        "mov $51, %eax;"
        "int $0x30;"
        "add $0x8, %esp;"
        );
    }*/

    /*
    asm(
        "mov $42, %eax;"
        "int $0x30;");
    */
    while(1);
}

/* Dividiert ein uint64 durch einen uint32 und gibt das Ergebnis zurück */
unsigned long long divmod(unsigned long long dividend, unsigned int divisor, unsigned int * remainder)
{
    unsigned int highword = dividend >> 32;
    unsigned int lowword = dividend & 0xffffffff;
    unsigned long long quotient;
    unsigned int rem;

    __asm__("div %%ecx\n\t"
        "xchg %%ebx, %%eax\n\t"
        "div %%ecx\n\t"
        "xchg %%edx, %%ebx"
        : "=A"(quotient), "=b"(rem)
        : "a"(highword), "b"(lowword), "c"(divisor), "d"(0)
        );

    if(remainder)
    {
        *remainder = rem;
    }

    return quotient;
}

void putn(unsigned long long x, int radix, int pad, char padchar)
{
    char b[65];
    char * r = b + 64; // r zeig auf das letzte Zeichen von b[]
    char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    unsigned int remainder;

    if(radix < 2 || radix > 36)
    {
        return;
    }

    *r-- = 0;

    do
    {
        x = divmod(x, radix, &remainder);
        *r-- = digits[remainder];
        pad--;
    }
    while(x > 0);

    while(pad-- > 0)
    {
//        puts(padchar);
    }

    puts(r + 1);
}

