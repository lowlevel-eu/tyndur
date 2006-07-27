/*
 * Funktionen zur Verwaltung von Interrupts
 *
 * Aenderungen:
 *
 * 2006-04-09: taljeth
 *   + Erste Version erstellta
 */

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

/**
 * Standardinterrupthandler
 *
 * Um zu verhindern, dass ein Stackframe aufgebaut wird oder der C-Compiler
 * ähnliche Dummheiten macht, wird die Funktion nur in C deklariert, aber
 * in Assembler implementiert.
 */
void null_handler();
asm("null_handler:\n"
    "pushl $0\n\t"
    "pushl $1337\n\t" /* Später gibts eh keinen "null handler" mehr, sondern nen anständigen GPF. Bis dahin bleiben wir 1337. */
    "jmp int_bottom\n\t");

/* Stub für eine Exception ohne Error-Code */
#define EXCEPTION_STUB(n) \
void exception_stub_ ## n (); \
__asm__("exception_stub_" #n ":\n\t" \
    "pushl $0\n\t" \
    "pushl $" #n "\n\t" \
    "jmp int_bottom\n\t");

/* Stub für eine Exception mit Fehlercode, der von der CPU auf den Stack gelegt wird */
#define EXCEPTION_STUB_ERROR_CODE(n) \
void exception_stub_ ## n (); \
__asm__("exception_stub_" #n ":\n\t" \
    "pushl $" #n "\n\t" \
    "jmp int_bottom\n\t");

/* Sichert die general purpose register und ruft den c-Handler auf */
__asm__("int_bottom:\n\t"
"pusha\n\t"
"push %ds\n\t"
"push %es\n\t"
"push %fs\n\t"
"push %gs\n\t"
"cld\n\t"
"mov $0x10, %ax\n\t"
"mov %ax, %ds\n\t"
"mov %ax, %es\n\t"

"call handle_int\n\t"

"pop %gs\n\t"
"pop %fs\n\t"
"pop %es\n\t"
"pop %ds\n\t"
"popa\n\t"

"add $8, %esp\n\t"
"iret\n\t");

EXCEPTION_STUB(0)
EXCEPTION_STUB(1)
EXCEPTION_STUB(2)
EXCEPTION_STUB(3)
EXCEPTION_STUB(4)
EXCEPTION_STUB(5)
EXCEPTION_STUB(6)
EXCEPTION_STUB(7)
EXCEPTION_STUB_ERROR_CODE(8)
EXCEPTION_STUB(9)
EXCEPTION_STUB_ERROR_CODE(10)
EXCEPTION_STUB_ERROR_CODE(11)
EXCEPTION_STUB_ERROR_CODE(12)
EXCEPTION_STUB_ERROR_CODE(13)
EXCEPTION_STUB_ERROR_CODE(14)
EXCEPTION_STUB(16)
EXCEPTION_STUB_ERROR_CODE(17)
EXCEPTION_STUB(18)
EXCEPTION_STUB(19)

struct int_stack_frame
{
    word gs  __attribute__((aligned(4)));
    word fs  __attribute__((aligned(4)));
    word es  __attribute__((aligned(4)));
    word ds  __attribute__((aligned(4)));
    dword edi;
    dword esi;
    dword ebp;
    dword esp;
    dword ebx;
    dword edx;
    dword ecx;
    dword eax;
    dword interrupt_number;
    dword error_code;
    dword eip;
    dword cs;
    dword eflags;
};

void handle_int(struct int_stack_frame isf)
{
    kprintf("int: #%d @ 0x%04x:0x%08x error code: 0x%08x\n", isf.interrupt_number, isf.cs, isf.eip, isf.error_code);
    kprintf("eax:0x%08x, ebx:0x%08x, ecx:0x%08x, edx:0x%08x\n", isf.eax, isf.ebx, isf.ecx, isf.edx);
    kprintf("ebp:0x%08x, esp:0x%08x, esi:0x%08x, edi:0x%08x\n", isf.ebp, isf.esp, isf.esi, isf.edi);
    kprintf("eflags:0x%08x, ds:0x%04x, es:0x%04x, fs:0x%04x, gs:0x%04x\n", isf.eflags, isf.ds, isf.es, isf.fs, isf.gs);
    for(;;);
}

/**
 * Legt eine IDT an der Adresse IDT_BASE an und initialisiert sie mit
 * Verweisen auf null_handler. Anschließend wird sie geladen.
 */
void init_idt() {
  int i;

  // Tabelle initialisieren
  for (i = 0; i < IDT_SIZE; i++)
  {
    set_intr(i, 0x08, &null_handler, 0, IDT_INTERRUPT_GATE);
  }

  // Register befüllen
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

  // Handler für den System Call installieren
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
