
#ifndef KPRINTF_H
#define KPRINTF_H

#include <stdint.h>
#include <stdarg.h>

void kprintf(char* format, ...);
void kaprintf(char* format, va_list args);

#endif /* ndef KPRINTF_H */
