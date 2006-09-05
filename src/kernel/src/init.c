/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Andreas Klebinger.
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
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
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

 /** 
  * Kernel-Initialisierung erfolgt hier,
  * die Funktion init wird von header.asm aufgerufen 
  */

#include "kernel.h"
#include "gdt.h"
#include "intr.h"
#include "kprintf.h"
#include "ports.h"
#include "multiboot.h"
#include "string.h"
#include "tss.h"

struct multiboot_info multiboot_info;

extern void init_phys_mem(vaddr_t mmap_addr, dword mmap_length);
extern void init_paging();
extern void init_scheduler(void);

void foo()
{
    int i;

    while(1)
    {
        kprintf("a");
        for(i = 0; i < 10000; i++) { __asm__ __volatile__("nop"); }
    }
}

void bar()
{
    int i;
    while(1)
    {
	kprintf("b");
        for(i = 0; i < 10000; i++) { __asm__ __volatile__("nop"); }
    }
}

void baz()
{
    int i;
    while(1)
    {
        kprintf("c");
        for(i = 0; i < 10000; i++) { __asm__ __volatile__("nop"); }
    }
}

void keyboard_handler(int irq)
{
    panic("funzt :)\n");
}

void init(int multiboot_magic, struct multiboot_info *boot_info)
{
    memcpy(&multiboot_info, boot_info, sizeof(struct multiboot_info));

    init_phys_mem(multiboot_info.mi_mmap_addr, multiboot_info.mi_mmap_length);
    init_paging();
    init_gdt();

    clrscr();

    init_idt();
    init_scheduler();
    
    kprintf("ok\n");

    kprintf("ok %x\n", alloc_kernel_page());
    
    for(;;);

    create_task(foo);
    create_task(bar);
    create_task(baz);

    enable_interrupts();

    request_irq(1, keyboard_handler); // den Tastatur IRQ fuer unseren Test-Handler anfordern.
    kprintf("Eine beliebige Taste druecken, um einen PANIC zu verursachen.\n");

    while(1)
    {
        __asm__ __volatile__("hlt");
    }
}

extern void stack_backtrace();

void panic(char * message, ...)
{
    int * args = ((int*)&message) + 1;
    disable_interrupts();
    kprintf("PANIC: ");
    kaprintf(message, &args);
    stack_backtrace();
    for(;;);
}
