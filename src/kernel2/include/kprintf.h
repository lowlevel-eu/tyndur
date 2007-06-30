#ifndef KPRINTF_H
#define KPRINTF_H

#include <stdint.h>

typedef __builtin_va_list va_list;
#define va_start(ap, f) __builtin_stdarg_start((ap),f)
#define va_arg __builtin_va_arg
#define va_end __builtin_va_end

void kprintf(char* format, ...);
void kaprintf(char* format, va_list args);

/// Ist nur zu Testzwecken hier
void kputn(unsigned long long x, int radix, int pad, char padchar);
#endif /* ndef KPRINTF_H */
