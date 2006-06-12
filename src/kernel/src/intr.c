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

gate_descriptor* idt;

/**
 * Standardinterrupthandler 
 *
 * Um zu verhindern, dass ein Stackframe aufgebaut wird oder der C-Compiler 
 * ‰hnliche Dummheiten macht, wird die Funktion nur in C deklariert, aber
 * in Assembler implementiert.
 */
void null_handler();
asm("null_handler:\n"
    "iret");

/**
 * Legt eine IDT an der Adresse IDT_BASE an und initialisiert sie mit
 * Verweisen auf null_handler. Anschlieﬂend wird sie geladen.
 */
void init_idt() {
  int i;

  // Tabelle initialisieren
  idt = (gate_descriptor*) IDT_BASE;
  for (i = 0; i < IDT_SIZE; i++)
  {
    set_intr(i, 1, &null_handler);
  }
  
  // Register bef¸llen
  struct {
    word size;
    word base;
    word base2;
  } idt_ptr = {
    .size  = IDT_SIZE*8,
    .base  = IDT_BASE & 0xFFFF,
    .base2 = IDT_BASE >> 16
  };
  
  asm("lidt %0" : : "g" (idt_ptr));
}    

/**
 * Setzt einen Eintrag in der IDT.
 *
 * \param intr Nummer des Interrupts
 * \param selector Nummer des Selektors (wird von dieser Funktion
 *   noch mit 8 multipliziert, um den richtigen GDT-Index zu errechnen)
 * \param handler Funktionspointer auf den Interrupthandler
 */
void set_intr(int intr, word selector, void* handler)
{
  idt[intr].lsb_handler = ((dword) handler) & 0xFFFF;
  idt[intr].msb_handler = (((dword) handler) >> 16) & 0xFFFF;
  idt[intr].access = 0x8E;
  idt[intr].selector = selector*8;
  idt[intr].reserved = 0;
}
