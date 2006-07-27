#ifndef INTR_H
#define INTR_H

#define IDT_SIZE 256

#define IDT_TASK_GATE 0x9
#define IDT_INTERRUPT_GATE 0xe
#define IDT_TRAP_GATE 0xf

#define IDT_DESC_PRESENT 0x80

void init_idt();
void set_intr(int intr, word selector, void* handler, int dpl, int type);

#endif
