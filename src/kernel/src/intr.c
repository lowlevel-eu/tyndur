/*
 * Funktionen zur Verwaltung von Interrupts
 *
 * Aenderungen:
 *
 * 2006-04-09: taljeth
 *   + Erste Version erstellta
 *
 * 2006-08-04: Jidder
 *   + IRQs
 */

#include "ports.h"
#include "types.h"
#include "intr.h"

typedef struct {
  word lsb_handler;
  word selector;
  byte reserved;
  byte access;
  word msb_handler;
} gate_descriptor;

gate_descriptor idt[IDT_SIZE];

pfIrqHandler irq_handlers[16] = { 0 }; // f¸r jeden IRQ genau ein Handler. Wenn es keinen Handler gibt, ist der entsprechende Wert 0.
void handle_irq(int irq);

extern void null_handler();

extern void exception_stub_0();
extern void exception_stub_1();
extern void exception_stub_2();
extern void exception_stub_3();
extern void exception_stub_4();
extern void exception_stub_5();
extern void exception_stub_6();
extern void exception_stub_7();
extern void exception_stub_8();
extern void exception_stub_9();
extern void exception_stub_10();
extern void exception_stub_11();
extern void exception_stub_12();
extern void exception_stub_13();
extern void exception_stub_14();
extern void exception_stub_16();
extern void exception_stub_17();
extern void exception_stub_18();
extern void exception_stub_19();

extern void irq_stub_0();
extern void irq_stub_1();
extern void irq_stub_2();
extern void irq_stub_3();
extern void irq_stub_4();
extern void irq_stub_5();
extern void irq_stub_6();
extern void irq_stub_7();
extern void irq_stub_8();
extern void irq_stub_9();
extern void irq_stub_10();
extern void irq_stub_11();
extern void irq_stub_12();
extern void irq_stub_13();
extern void irq_stub_14();
extern void irq_stub_15();

//extern void system_call_stub();

struct int_stack_frame
{
    word gs  __attribute__((aligned(4)));
    word fs  __attribute__((aligned(4)));
    word es  __attribute__((aligned(4)));
    word ds  __attribute__((aligned(4)));
    dword edi;
    dword esi;
    dword ebp;
    dword esp_;
    dword ebx;
    dword edx;
    dword ecx;
    dword eax;
    dword interrupt_number;
    dword error_code;
    dword eip;
    dword cs;
    dword eflags;
    dword esp;
    dword ss;
};

struct task
{
    dword esp;
    dword cr3;
    dword slices;
};

#define MAX_TASKS 3
struct task tasks[MAX_TASKS];

struct task * current_task = &tasks[0];

/*
 * gibt einen neuen Wert f¸r esp zur¸ck
 */
dword handle_int(dword esp)
{
    struct int_stack_frame * isf = (struct int_stack_frame *)esp;

    current_task->esp = esp;

    if(isf->interrupt_number <= 31)
    {
        // Eine Exception.
        // TODO: CLS und Bildschirmhintergrund auf blau und Schriftfarbe auf weiss stellen.
        kprintf("Es wurde ein Problem festgestellt. LOST wurde heruntergefahren, damit der\n"
                "Computer nicht besch‰digt wird.\n"
                "\n"
                "Exception #0x%02d\n", isf->interrupt_number);

        kprintf("int: #%d @ 0x%04x:0x%08x ss:esp= 0x%04x:0x%08x error code: 0x%08x\n", isf->interrupt_number, isf->cs, isf->eip, isf->ss, isf->esp, isf->error_code);
        kprintf("eax:0x%08x, ebx:0x%08x, ecx:0x%08x, edx:0x%08x\n", isf->eax, isf->ebx, isf->ecx, isf->edx);
        kprintf("ebp:0x%08x, esp:0x%08x, esi:0x%08x, edi:0x%08x\n", isf->ebp, isf->esp, isf->esi, isf->edi);
        kprintf("eflags:0x%08x, ds:0x%04x, es:0x%04x, fs:0x%04x, gs:0x%04x\n", isf->eflags, isf->ds, isf->es, isf->fs, isf->gs);

        for(;;);
    }
    else if(isf->interrupt_number <= 47)
    {
        // Ein IRQ
        handle_irq(isf->interrupt_number - IRQ_BASE);
    }
    else if(isf->interrupt_number == 48)
    {
        // Ein Syscall
        // TODO: Programmieren
    }
    else
    {
        // TODO: Abst¸rzen, denn diese Interrupts d¸rften nicht aufgerufen werden
    }

    //current_task = schedule();

   // for(;;);

    return current_task->esp;
}

/**
 * Legt eine IDT an und installiert die Exception Handler. Nicht genutzte
 * Eintr‰ge werden mit Verweisen auf den Null-Handler initalisiert.
 * Anschlieﬂend wird die IDT geladen.
 */
void init_idt() {
  int i;

  // Tabelle initialisieren
  for (i = 0; i < IDT_SIZE; i++)
  {
    set_intr(i, 0x08, &null_handler, 0, IDT_INTERRUPT_GATE);
  }

  // Register bef¸llen
  struct {
    word size;
    dword base;
  }  __attribute__((packed)) idt_ptr = {
    .size  = IDT_SIZE*8 - 1,
    .base  = (dword)idt,
  };

  // Exception Handler installieren
  set_intr(0, 0x08, exception_stub_0, 0, IDT_INTERRUPT_GATE);
  set_intr(1, 0x08, exception_stub_1, 0, IDT_INTERRUPT_GATE);
  set_intr(2, 0x08, exception_stub_2, 0, IDT_INTERRUPT_GATE);
  set_intr(3, 0x08, exception_stub_3, 0, IDT_INTERRUPT_GATE);
  set_intr(4, 0x08, exception_stub_4, 0, IDT_INTERRUPT_GATE);
  set_intr(5, 0x08, exception_stub_5, 0, IDT_INTERRUPT_GATE);
  set_intr(6, 0x08, exception_stub_6, 0, IDT_INTERRUPT_GATE);
  set_intr(7, 0x08, exception_stub_7, 0, IDT_INTERRUPT_GATE);
  set_intr(8, 0x08, exception_stub_8, 0, IDT_INTERRUPT_GATE);
  set_intr(9, 0x08, exception_stub_9, 0, IDT_INTERRUPT_GATE);
  set_intr(10, 0x08, exception_stub_10, 0, IDT_INTERRUPT_GATE);
  set_intr(11, 0x08, exception_stub_11, 0, IDT_INTERRUPT_GATE);
  set_intr(12, 0x08, exception_stub_12, 0, IDT_INTERRUPT_GATE);
  set_intr(13, 0x08, exception_stub_13, 0, IDT_INTERRUPT_GATE);
  set_intr(14, 0x08, exception_stub_14, 0, IDT_INTERRUPT_GATE);
  set_intr(16, 0x08, exception_stub_16, 0, IDT_INTERRUPT_GATE);
  set_intr(17, 0x08, exception_stub_17, 0, IDT_INTERRUPT_GATE);
  set_intr(18, 0x08, exception_stub_18, 0, IDT_INTERRUPT_GATE);
  set_intr(19, 0x08, exception_stub_19, 0, IDT_INTERRUPT_GATE);

  set_intr(IRQ_BASE + 0, 0x08, irq_stub_0, 0, IDT_INTERRUPT_GATE);
  set_intr(IRQ_BASE + 1, 0x08, irq_stub_1, 0, IDT_INTERRUPT_GATE);
  set_intr(IRQ_BASE + 2, 0x08, irq_stub_2, 0, IDT_INTERRUPT_GATE);
  set_intr(IRQ_BASE + 3, 0x08, irq_stub_3, 0, IDT_INTERRUPT_GATE);
  set_intr(IRQ_BASE + 4, 0x08, irq_stub_4, 0, IDT_INTERRUPT_GATE);
  set_intr(IRQ_BASE + 5, 0x08, irq_stub_5, 0, IDT_INTERRUPT_GATE);
  set_intr(IRQ_BASE + 6, 0x08, irq_stub_6, 0, IDT_INTERRUPT_GATE);
  set_intr(IRQ_BASE + 7, 0x08, irq_stub_7, 0, IDT_INTERRUPT_GATE);
  set_intr(IRQ_BASE + 8, 0x08, irq_stub_8, 0, IDT_INTERRUPT_GATE);
  set_intr(IRQ_BASE + 9, 0x08, irq_stub_9, 0, IDT_INTERRUPT_GATE);
  set_intr(IRQ_BASE + 10, 0x08, irq_stub_10, 0, IDT_INTERRUPT_GATE);
  set_intr(IRQ_BASE + 11, 0x08, irq_stub_11, 0, IDT_INTERRUPT_GATE);
  set_intr(IRQ_BASE + 12, 0x08, irq_stub_12, 0, IDT_INTERRUPT_GATE);
  set_intr(IRQ_BASE + 13, 0x08, irq_stub_13, 0, IDT_INTERRUPT_GATE);
  set_intr(IRQ_BASE + 14, 0x08, irq_stub_14, 0, IDT_INTERRUPT_GATE);
  set_intr(IRQ_BASE + 15, 0x08, irq_stub_15, 0, IDT_INTERRUPT_GATE);


  // PIC initalisieren
  outb_wait(PIC1_COMMAND, ICW1_INIT + ICW1_ICW4);
  outb_wait(PIC2_COMMAND, ICW1_INIT + ICW1_ICW4);

  // nach IRQ_BASE bis IRQ_BASE + 0xf remappen
  outb_wait(PIC1_DATA, IRQ_BASE);
  outb_wait(PIC2_DATA, IRQ_BASE + 8);

  // den Slave auf IRQ 2 setzen
  outb_wait(PIC1_DATA, 4);
  outb_wait(PIC2_DATA, 2);

  outb_wait(PIC1_DATA, ICW4_8086);
  outb_wait(PIC2_DATA, ICW4_8086);

  // alle irqs aktivieren
  outb_wait(PIC1_DATA, 0x00);
  outb_wait(PIC2_DATA, 0x00);

  // Handler f¸r den System Call installieren
  //set_intr(SYS_CALL_INTERRUPT, 0x08, system_call_stub, 3, IDT_TRAP_GATE);

  // IDT laden
  asm("lidt %0" : : "g" (idt_ptr));
}

/**
 * Setzt einen Eintrag in der IDT.
 *
 * \param intr Nummer des Interrupts
 * \param selector Code-Selektor
 * \param handler Funktionspointer auf den Interrupthandler
 * \param dpl Descriptor Privilege Level
 * \param type Deskriptor Typ
 */
void set_intr(int intr, word selector, void* handler, int dpl, int type)
{
  idt[intr].lsb_handler = ((dword) handler) & 0xFFFF;
  idt[intr].msb_handler = (((dword) handler) >> 16) & 0xFFFF;
  idt[intr].access = IDT_DESC_PRESENT | ((dpl & 3) << 5) | type;
  idt[intr].selector = selector;
  idt[intr].reserved = 0;
}

int request_irq(int irq, void * handler)
{
    if(irq < 0 || irq > 15)
    {
        return 1;
    }

    if(irq_handlers[irq] != 0)
    {
        return 1;
    }

    irq_handlers[irq] = handler;

    return 0;
}

int release_irq(int irq)
{
    if(irq < 0 || irq > 15)
    {
        return 1;
    }

    irq_handlers[irq] = 0;

    return 0;
}


void handle_irq(int irq)
{
    if(irq_handlers[irq] == 0)
    {
        panic("unhandled irq %d\n\t", irq);
    }

    irq_handlers[irq](irq);

    if(irq >= 8)
    {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}
