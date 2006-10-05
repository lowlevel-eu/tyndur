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
#include "kprintf.h"
#include "paging.h"

struct multiboot_info multiboot_info;

extern void init_phys_mem(vaddr_t mmap_addr, dword mmap_length);
extern void init_paging();
extern void init_console();
extern void init_scheduler(void);
extern void init_stack_backtrace(void);

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
    init_console();

    memcpy(&multiboot_info, boot_info, sizeof(struct multiboot_info));

    init_phys_mem(multiboot_info.mi_mmap_addr, multiboot_info.mi_mmap_length);
    init_paging();
    init_gdt();

    init_stack_backtrace();
    
    init_idt();
    init_scheduler();

     void * x = phys_alloc_page(); 
     kprintf("phys = %x\n", x); 
     map_page(kernel_page_directory, (vaddr_t)0x2000000, x, PTE_W|PTE_P|PTE_U);
     
     void * y = phys_alloc_page(); 
     kprintf("phys = %x\n", y); 
     map_page((vaddr_t) 0x2000000, (vaddr_t)0x2100000, y, PTE_W|PTE_P|PTE_U);
    
    /* keine angst. nur zum test/demonstieren ;) */
    kprintf("physical memory manager testfaelle:\n");
    void * ptr;
#define TALLOC(num) ({void * t = phys_alloc_page_range(num); kprintf("alloc %d pages: %x\n", num, t); t;})
#define TFREE(p, num) kprintf("freeing %d pages at %x\n", num, p); phys_free_page_range((void*)p, num);
    ptr = TALLOC(80);
    TALLOC(48);
    TFREE(ptr, 80);
    TALLOC(48);
    TALLOC(80);
    TALLOC(24);

    kprintf("LOST wurde erfolgreich gestartet. Sie koennen Ihren PC jetzt ausschalten.\n");

    enable_interrupts();

    while(1)
    {
        __asm__ __volatile__("hlt");
    }
}

extern void stack_backtrace();

__attribute__((noreturn)) void panic(char * message, ...)
{
    int * args = ((int*)&message) + 1;
    disable_interrupts();

    kprintf("\n"
      "\033[1;37m\033[41m" // weiss auf rot
      "PANIC: ");
    kaprintf(message, &args);
    kprintf("\n");

    stack_backtrace();

    while(1) {
      __asm__ __volatile__("hlt");
    }
}
