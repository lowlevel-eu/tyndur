#ifndef INTR_H
#define INTR_H

#define IDT_SIZE 64

// FIXME: Aus kernel_memory.h beziehen
#define IDT_BASE 0x700

void init_idt();
void set_intr(int intr, word selector, void* handler);

#endif
