#ifndef INTR_H
#define INTR_H

#include <stdint.h>

#include "tasks.h"

#define SYSCALL_INTERRUPT 0x30

#define IDT_SIZE 256

#define IDT_TASK_GATE 0x9
#define IDT_INTERRUPT_GATE 0xe
#define IDT_TRAP_GATE 0xf

#define IDT_DESC_PRESENT 0x80

void init_idt(void);
void set_intr(int intr, uint16_t selector, void* handler, int dpl, int type);
void set_intr_handling_task(uint8_t intr, struct task * task);
void remove_intr_handling_task(struct task* task);

typedef void(*pfIrqHandler)(int, uint32_t*);

#define IRQ_BASE 0x20

#define PIC1            0x20           /* IO base address for master PIC */
#define PIC2            0xA0           /* IO base address for slave PIC */
#define PIC1_COMMAND    PIC1
#define PIC1_DATA       (PIC1+1)
#define PIC2_COMMAND    PIC2
#define PIC2_DATA       (PIC2+1)
#define PIC_EOI         0x20            /* End - of - interrupt command code */

#define ICW1_ICW4       0x01            /* ICW4 (not) needed */
#define ICW1_SINGLE     0x02            /* Single (cascade) mode */
#define ICW1_INTERVAL4  0x04            /* Call address interval 4 (8) */
#define ICW1_LEVEL      0x08            /* Level triggered (edge) mode */
#define ICW1_INIT       0x10            /* Initialization - required! */

#define ICW4_8086       0x01            /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02            /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08            /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C            /* Buffered mode/master */
#define ICW4_SFNM       0x10            /* Special fully nested (not) */

int request_irq(int irq, void * handler);
int release_irq(int irq);

inline static void enable_interrupts(void) { __asm__ __volatile__("sti"); }
inline static void disable_interrupts(void) { __asm__ __volatile__("cli"); }

void disable_irq(uint8_t irq);
void enable_irq(uint8_t irq);

struct int_stack_frame
{
    uint16_t gs  __attribute__((aligned(4)));
    uint16_t fs  __attribute__((aligned(4)));
    uint16_t es  __attribute__((aligned(4)));
    uint16_t ds  __attribute__((aligned(4)));
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t interrupt_number;
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
};


#endif
