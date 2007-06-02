#ifndef KPRINTF_H
#define KPRINTF_H

#include <stdint.h>

extern void kprintf(char * format, ...);
extern void kaprintf(char * format, int** args);

/// Ist nur zu Testzwecken hier
void kputn(unsigned long long x, int radix, int pad, char padchar);
#endif /* ndef KPRINTF_H */
