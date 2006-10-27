#include "types.h"

void putn(unsigned long long x, int radix, int pad, char padchar);
inline void puts(char* str);


void _start() {
    char *hw = "Hello world!";
    dword ptr;

    int i;

    asm(
        "push %0;"
        "xor %%eax, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
        : : "r" (hw));
    putn(1337, 10, 10, '.');
    puts("\n");

    for (i = 0; i < 10; i++) {
    asm(
        "pushl %0;"
        "pushl %1;"
        "mov $1, %%eax;"
        "int $0x30;"
        "add $0x8, %%esp;"
        : "=a" (ptr) : "r" (0x1234), "r" (0xf5));

    puts("\n[");
    putn(i, 10, 2, ' ');
    puts("] Reservierter Speicher an ");
    putn(ptr, 16, 10, '.');
    }

    puts("\nBlubb!");
    
    asm(
        "mov $42, %eax;"
        "int $0x30;");

    while(1);
}

inline void puts(char* str) {
    asm("push %0;"
        "xor %%eax, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp" : : "r" (str));
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

