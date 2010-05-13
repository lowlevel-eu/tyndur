#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define CR0_PE		0x00000001 // Protection Enable
#define CR0_PG		0x80000000 // Paging

#define CR4_PSE		0x00000010 // Page Size Extensions

// eflags register
#define FL_TF		0x00000100 // Trap Flag
#define FL_IF		0x00000200 // Interrupt Flag
#define FL_IOPL0	0x00000000 // I/O Privilege Level 0
#define FL_IOPL1	0x00001000 // I/O Privilege Level 1
#define FL_IOPL2	0x00002000 // I/O Privilege Level 2
#define FL_IOPL3	0x00003000 // I/O Privilege Level 3
#define FL_NT		0x00004000 // Nested Task
#define FL_RF		0x00010000 // Resume Flag
#define FL_VM		0x00020000 // Virtual 8086 mode

static inline uint32_t read_cr2(void)
{
    uint32_t ret;
    __asm volatile ("mov %%cr2, %0" : "=r"(ret));
    return ret;
}

#endif /* ndef CPU_H */
