/*  
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */  

/*
 * Funktionen zur Verwaltung von Interrupts
 */

#include <stdint.h>

#include "cpu.h"
#include "ports.h"
#include "types.h"
#include "kernel.h"
#include "intr.h"
#include "tss.h"
#include "kprintf.h"
#include "paging.h"
#include "string.h"
#include "syscall.h"
#include "tasks.h"
#include "vmm.h"
#include <lost/config.h>
#include "debug.h"
#include "rpc.h"
#include "vm86.h"
#include "tss.h"

#define MAX_INTERRUPTS 4

typedef struct {
  uint16_t lsb_handler;
  uint16_t selector;
  uint8_t reserved;
  uint8_t access;
  uint16_t msb_handler;
} gate_descriptor;

gate_descriptor idt[IDT_SIZE];

/**
 * Fuer jeden IRQ genau ein Handler. Wenn es keinen Handler gibt, ist der
 * entsprechende Wert 0.
 */
pfIrqHandler irq_handlers[16] = { 0 };

static struct task * intr_handling_task[256][MAX_INTERRUPTS] = { { NULL } };

void handle_irq(int irq, uint32_t* esp);

extern void null_handler(void);

extern void exception_stub_0(void);
extern void exception_stub_1(void);
extern void exception_stub_2(void);
extern void exception_stub_3(void);
extern void exception_stub_4(void);
extern void exception_stub_5(void);
extern void exception_stub_6(void);
extern void exception_stub_7(void);
extern void exception_stub_8(void);
extern void exception_stub_9(void);
extern void exception_stub_10(void);
extern void exception_stub_11(void);
extern void exception_stub_12(void);
extern void exception_stub_13(void);
extern void exception_stub_14(void);
extern void exception_stub_16(void);
extern void exception_stub_17(void);
extern void exception_stub_18(void);
extern void exception_stub_19(void);

extern void irq_stub_0(void);
extern void irq_stub_1(void);
extern void irq_stub_2(void);
extern void irq_stub_3(void);
extern void irq_stub_4(void);
extern void irq_stub_5(void);
extern void irq_stub_6(void);
extern void irq_stub_7(void);
extern void irq_stub_8(void);
extern void irq_stub_9(void);
extern void irq_stub_10(void);
extern void irq_stub_11(void);
extern void irq_stub_12(void);
extern void irq_stub_13(void);
extern void irq_stub_14(void);
extern void irq_stub_15(void);

extern void syscall_stub(void);

extern void stack_backtrace_ebp(uint32_t start_ebp, uint32_t start_eip);

void handle_exception(uint32_t* esp);

void send_irqs(uint32_t* esp);
static uint32_t irqs_to_send[16][MAX_INTERRUPTS];
extern uint32_t kernel_pd_id;

/*
 * gibt einen neuen Wert fuer esp zurck
 */
uint32_t handle_int(uint32_t esp)
{
    struct int_stack_frame * isf = (struct int_stack_frame *)esp;
    struct task* old_task = current_task;

//	kprintf("\nTask switch!  esp alter Task:0x%08x", isf->esp);
	
	if(isf->interrupt_number <= 31)
    {
        handle_exception(&esp);
    }
    else if(isf->interrupt_number <= 47)
    {
        // Ein IRQ
        handle_irq(isf->interrupt_number - IRQ_BASE, &esp);
		isf = (struct int_stack_frame *)esp;
    }
    else if(isf->interrupt_number == 48)
    {
        // Ein Syscall
        syscall((struct int_stack_frame **) &esp);
    }
    else
    {
        // TODO: Abstuerzen, denn diese Interrupts duerften nicht aufgerufen werden
    }

    send_irqs(&esp);
    //current_task = schedule();

   // for(;;);

    if ((current_task != NULL) && (old_task != current_task)) {
        //tss.esp = current_task->kernel_stack;
        isf = (struct int_stack_frame *)esp;
        //kprintf("  esp neuer Task:0x%08x", isf->esp);

        tss.esp0 = current_task->kernel_stack;
        // sicherstellen, dass der Kernel Adressraum im Page Directory des Tasks korrekt gemappt ist
        //kprintf("PID=%d  cr3=0x%08x  kernel_pd=0x%08x eip=0x%08x\n", 
        //  current_task->pid, current_task->cr3, (uint32_t)kernel_page_directory, isf->eip);
        //memcpy((void*)current_task->cr3, kernel_page_directory, 1024);

        // Aktuelles Kernel-PD kopieren, wenn noetig
        if (kernel_pd_id != current_task->pd_id) {
            memcpy((void*) current_task->cr3, kernel_page_directory, 1020);
            current_task->pd_id = kernel_pd_id;
        }

        // Page Directory des neuen Tasks laden
        __asm("mov %0, %%cr3" : : "r"(resolve_vaddr(kernel_page_directory, current_task->cr3)));
    }

    if (current_task && (current_task->status == TS_WAIT_FOR_RPC)) {
        current_task->status = TS_RUNNING;
    }

    //kprintf("int_handler: return %08x\n", esp);
    //kprintf("int_handler: eip=%08x\n", isf->eip);
    return esp;
}

/**
 * Legt eine IDT an und installiert die Exception Handler. Nicht genutzte
 * Eintraege werden mit Verweisen auf den Null-Handler initalisiert.
 * Anschliessend wird die IDT geladen.
 */
void init_idt() {
  int i;

  // Tabelle initialisieren
  for (i = 0; i < IDT_SIZE; i++)
  {
    set_intr(i, 0x08, &null_handler, 0, IDT_INTERRUPT_GATE);
  }

  // Register befuellen
  struct {
    uint16_t size;
    uint32_t base;
  }  __attribute__((packed)) idt_ptr = {
    .size  = IDT_SIZE*8 - 1,
    .base  = (uint32_t)idt,
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

  // Handler fuer den System Call installieren
  // TODO: Sollte eigentlich ein IDT_TRAP_GATE sein, aber irgendwo ist da
  // noch ein Problem, das dann fuer einen Page Fault sorgt, wenn ich zu
  // schnell tippe...
  set_intr(SYSCALL_INTERRUPT, 0x08, syscall_stub, 3, IDT_INTERRUPT_GATE);
  //set_intr(SYSCALL_INTERRUPT, 0x08, syscall_stub, 3, IDT_TRAP_GATE);

  // IDT laden
  asm("lidt %0" : : "m" (idt_ptr));
}

void disable_irq(uint8_t irq)
{
    if (irq < 8) {
        outb(PIC1_DATA, inb(PIC1_DATA) | (1 << irq));
    } else if (irq < 16) {
        outb(PIC2_DATA, inb(PIC2_DATA) | (1 << (irq - 8)));
    } else {
        panic("Ungueltiger IRQ %d", irq);
    }
}

void enable_irq(uint8_t irq)
{
    if (irq < 8) {
        outb(PIC1_DATA, inb(PIC1_DATA) & ~(1 << irq));
    } else if (irq < 16) {
        outb(PIC2_DATA, inb(PIC2_DATA) & ~(1 << (irq - 8)));
    } else {
        panic("Ungueltiger IRQ %d", irq);
    }
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
void set_intr(int intr, uint16_t selector, void* handler, int dpl, int type)
{
  idt[intr].lsb_handler = ((uint32_t) handler) & 0xFFFF;
  idt[intr].msb_handler = (((uint32_t) handler) >> 16) & 0xFFFF;
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


void handle_irq(int irq, uint32_t* esp)
{
    int i;

    //if (irq > 0) {
    //    kprintf("Interrupt 0x%x\n", irq + IRQ_BASE);
    //}

    /*if (irq > 0)
    {
        uint8_t pic = (irq < 8 ? PIC1 : PIC2);
        outb(pic + 3, 0x03);
        if ((inb(pic) & (1 << (irq % 8))) == 0) {
            kprintf("Spurious IRQ %d\n", irq);
        }
    }*/

    // IRQs 7 und 15 koennen unabsichtlich aufgerufen werden
    // In diesem Fall beim PIC nachschauen, ob wirklich was zu verarbeiten 
    // ist, ansonsten gleich wieder abbrechen.
    if ((irq == 7) || (irq == 15))
    {
        uint8_t pic = (irq < 8 ? PIC1 : PIC2);

        outb(pic, 0x0B);
        if ((inb(pic) & 0x80) == 0) {
            kprintf("Spurious IRQ %d\n", irq);

            // TODO Eigentlich sollte man hier keinen EOI brauchen, aber qemu
            // scheint ihn zu brauchen
            goto send_eoi;
        }
    }

    for (i = 0; i < MAX_INTERRUPTS; i++) {
        if (intr_handling_task[irq + IRQ_BASE][i] != NULL) {
            i = -1;
            break;
        }
    }
    if (i == -1)
    {
        //kprintf("IRQ-Verarbeitung durch ein Modul\n");
        for (i = 0; i < MAX_INTERRUPTS; i++) {
            if (intr_handling_task[irq + IRQ_BASE][i] != NULL) {
                irqs_to_send[irq][i]++;
            }
        }

        //printf("disable IRQ %d\n", irq);
        disable_irq(irq);
    } 
    else if (irq_handlers[irq] != 0)
    {
        irq_handlers[irq](irq, esp);
    }

send_eoi:
    if(irq >= 8)
    {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void set_intr_handling_task(uint8_t intr, struct task * task)
{
    int i;

    //kprintf("Interrupt %d wird jetzt von Task %d behandelt\n", intr, task->pid);
    for (i = 0; i < MAX_INTERRUPTS; i++) {
        if ((intr_handling_task[intr][i] == NULL) ||
            (i == MAX_INTERRUPTS - 1))
        {
            intr_handling_task[intr][i] = task;
            if (intr >= IRQ_BASE && intr < IRQ_BASE + 16) {
                irqs_to_send[intr - IRQ_BASE][i] = 0;
            }
            break;
        }
    }
}

void remove_intr_handling_task(struct task* task)
{
    int i, intr;

    for (intr = 0; intr < 256; intr++) {
        for (i = 0; i < MAX_INTERRUPTS; i++) {
            if (intr_handling_task[intr][i] == task) {
                intr_handling_task[intr][i] = NULL;
            }
        }
    }
}

void handle_exception(uint32_t* esp)
{
    struct int_stack_frame * isf = *((struct int_stack_frame **)esp);

    uint32_t cr2;
    intptr_t diff;

    switch(isf->interrupt_number)
    {
        case 13:
            // Falls der Task seine IO-Bitmap noch nicht bekommen hat,
            // erledigen wir das jetzt und lassen ihn dann nochmal versuchen.
            if (tss.io_bit_map_offset == TSS_IO_BITMAP_NOT_LOADED) {
                set_io_bitmap();
                return;
            }
            break;

        case 14:
            cr2 = read_cr2();
            diff = (cr2 >> PAGE_SHIFT) - (isf->esp >> PAGE_SHIFT);
            // Ueberpruefen ob der Pagefault durch einen Stackoverflow
            // hervorgerufen wurde
            if ((diff >= -2) && (diff <= 2)
                && (current_task != NULL)
                && ((void*)cr2 >= current_task->user_stack_bottom - 0x1000000))
            {
                increase_user_stack_size(current_task, 1);
                return;
            }

            kprintf("\033[1;37m\033[41mPage Fault: 0x%x\033[0;37m\033[40m", cr2);
            break;
    }

    // Pruefen, ob ein VM86-Task die Exception ausgeloest hat, und evtl reagieren
    if (isf->eflags & 0x20000) {
        if (vm86_exception(esp)) {
            return;
        }
    }
    // Eine Exception. Fehlermeldung ausgeben und weg.
    // Falls die Exception im Kernel aufgetreten ist, richtig weg, sonst nur
    // den schuldigen Task beenden.
    if (((isf->cs & 0x03) == 0) && !(isf->eflags & 0x20000)) {
        kprintf("\n");
        kprintf("\033[1;37m\033[41m"); // weiss auf rot
        kprintf("Es wurde ein Problem festgestellt. tyndur wurde heruntergefahren, damit der\n"
                "Computer nicht beschaedigt wird.\n"
                "\n"
                "Exception #%02d, int: #%d @ 0x%04x:0x%08x, PID %d, %s\n", 
                isf->interrupt_number, isf->interrupt_number, isf->cs, isf->eip, current_task ? current_task->pid : 0, "(deaktiviert)" /*current_task ? current_task->cmdline : "(kernel)"*/);
        kprintf("ss:esp= 0x%04x:0x%08x error code: 0x%08x\n", isf->ss, isf->esp, isf->error_code);
        kprintf("eax:0x%08x, ebx:0x%08x, ecx:0x%08x, edx:0x%08x\n", isf->eax, isf->ebx, isf->ecx, isf->edx);
        kprintf("ebp:0x%08x, esp:0x%08x, esi:0x%08x, edi:0x%08x\n", isf->ebp, isf->esp, isf->esi, isf->edi);
        kprintf("eflags:0x%08x, ds:0x%04x, es:0x%04x, fs:0x%04x, gs:0x%04x\n", isf->eflags, isf->ds, isf->es, isf->fs, isf->gs);
        #ifdef CONFIG_DEBUG_LAST_SYSCALL
        {
            uint32_t i;

            kprintf("Letzter Syscall: %d ", debug_last_syscall_no);
            for (i = 0; i < DEBUG_LAST_SYSCALL_DATA_SIZE; i++) {
                kprintf("0x%08x ", debug_last_syscall_data[i]);
            }
            kprintf("\n");
        }
        #endif 

        if(debug_test_flag(DEBUG_FLAG_STACK_BACKTRACE))
        {
            stack_backtrace_ebp(isf->ebp, isf->eip);
        }

        while(1) {
          asm("cli; hlt");
        }
    } else {
        kprintf(
            "\n\033[1;37m\033[41mDer folgende Task wurde aufgrund einer "
            "unbehandelten Ausnahme abgebrochen:\n%s\n\n"
            "Exception #%02d, int: #%d @ 0x%04x:0x%08x, PID %d\n"
            "ss:esp= 0x%04x:0x%08x error code: 0x%08x\n"
            "eax:0x%08x, ebx:0x%08x, ecx:0x%08x, edx:0x%08x\n"
            "ebp:0x%08x, esp:0x%08x, esi:0x%08x, edi:0x%08x\n"
            "eflags:0x%08x, ds:0x%04x, es:0x%04x, fs:0x%04x, gs:0x%04x\n"
            "\033[0;37m\033[40m",
            
            (current_task == 0 
                ? "Kernel-Initialisierung"
                : (current_task->cmdline != NULL 
                    ? current_task->cmdline 
                    : "Unbekannter Task")),

            isf->interrupt_number, isf->interrupt_number, isf->cs, isf->eip, 
            current_task ? current_task->pid : 0,
            isf->ss, isf->esp, isf->error_code, 
            isf->eax, isf->ebx, isf->ecx, isf->edx, 
            isf->ebp, isf->esp, isf->esi, isf->edi, 
            isf->eflags, isf->ds, isf->es, isf->fs, isf->gs
        );

        abort_task("Unbehandelte Ausnahme.");
    }
}

void send_irqs(uint32_t* esp) 
{
    uint32_t irq;
    int task;

    for (irq = 0; irq < 16; irq++)
    {
        uint32_t rpc_data = irq + IRQ_BASE;
        for (task = 0; task < MAX_INTERRUPTS; task++) {
            if (intr_handling_task[irq + IRQ_BASE][task] == NULL) {
                continue;
            }
            while (irqs_to_send[irq][task] > 0) {
                schedule_to_task(intr_handling_task[irq + IRQ_BASE][task], esp);
                if (!fastrpc_irq(intr_handling_task[irq + IRQ_BASE][task], 0, 0, 4,
                    (char*) &rpc_data, irq))
                {
                    //kprintf("SYSCALL_RPC = FALSE");
                    break;
                }
                //kprintf("IRQ-Handler aufgerufen");
                irqs_to_send[irq][task]--;
            }
        }
    }
}
