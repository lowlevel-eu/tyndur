#ifndef KPRINTF_H
#define KPRINTF_H

#include <stdint.h>
#include <stdarg.h>

void kprintf(char* format, ...);
void kaprintf(char* format, va_list args);

/// Ist nur zu Testzwecken hier
void kputn(unsigned long long x, int radix, int pad, char padchar);
#endif /* ndef KPRINTF_H */
