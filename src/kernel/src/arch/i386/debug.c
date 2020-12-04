/*
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Burkhard Weseloh.
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

#include <elf32.h>
#include <stdint.h>
#include <string.h>
#include <collections.h>

#include <lost/config.h>

#include "kprintf.h"
#include "multiboot.h"
#include "debug.h"
#include "mm.h"
#include "cpu.h"
#include "tasks.h"

// TODO: Ueberarbeiten und ein bisschen weniger geizig sein mit den Kommentaren
// ;-)

#undef USE_ELF_SYM

#ifdef USE_ELF_SYM
static Elf32_Shdr * symtab = 0; // Zeiger auf den ELF-Header-Eintrag der 
                                // Symbol Tabelle ...
static Elf32_Shdr * strtab = 0; // ... und der String Tabelle

/**
 * Gibt einen Zeiger auf das Symbol mit der größten Adresse kleiner als addr 
 * zurück. Wenn kein Symbol bestimmt werden kann, wird 0 zurück gegeben.
 */
static Elf32_Sym* elf_find_sym(uint32_t addr)
{
    Elf32_Sym* sym;
    int i;

    if (symtab == 0) {
        return 0;
    }

    sym = (Elf32_Sym*)symtab->sh_addr;

    for (i = 0; i < symtab->sh_size / sizeof(Elf32_Sym); i++) {
        if(addr >= (uintptr_t)sym[i].st_value && addr < 
            (uintptr_t)sym[i].st_value + sym[i].st_size)
        {
            return sym + i;
        }
    }

    return 0;
}

/**
 * Gibt den String zum Index in die String Tabelle des Kernels zurück.
 */
static char * elf_get_str(Elf32_Word index)
{
    if (strtab == 0) {
        return "";
    }

    return (char*)strtab->sh_addr + index;
}

#endif

/**
 * Mappt den ELF-Header, sowie die ELF-Symbol-Tabelle und die dazugehörige
 * String-Tabelle, wenn sie vorhanden sind.
 */
void init_stack_backtrace(void)
{
  #ifdef USE_ELF_SYM
    Elf32_Shdr * elfshdr = (Elf32_Shdr *)multiboot_info.mi_elfshdr_addr;
    int i;

    /* ELF Header mappen */
    // XXX map_page_range(kernel_page_directory, elfshdr, elfshdr, PTE_W | PTE_P /*| PTE_U*/, NUM_PAGES(multiboot_info.mi_elfshdr_num * sizeof(Elf32_Shdr)));

    if (multiboot_info.mi_flags & MULTIBOOT_INFO_HAS_ELF_SYMS) {
        /* Symbol Table und String Table suchen */
        for (i = 0; i < multiboot_info.mi_elfshdr_num; i++) {
            if (elfshdr[i].sh_type == SHT_SYMTAB) {
                symtab = &elfshdr[i];
                strtab = &elfshdr[symtab->sh_link];
                
                break;
            }
        }
    }

    if (symtab != 0 && strtab != 0) {
        /* Symbol Tabelle und String Tabelle mappen */

        // XXX kernel_identity_map((paddr_t)symtab->sh_addr, symtab->sh_size);
        // XXX kernel_identity_map((paddr_t)strtab->sh_addr, strtab->sh_size);
    }
  #endif
}


void stack_backtrace(uint32_t start_ebp, uint32_t start_eip)
{
    struct stack_frame
    {
        struct stack_frame * ebp;
        uintptr_t eip;
       /* uint32_t args[0];*/
    } * stack_frame;
  #ifdef USE_ELF_SYM
    Elf32_Sym * sym;
  #endif

    kprintf("stack backtrace:\n");

    if (start_ebp != 0)
    {
        kprintf("ebp %8x eip %8x", start_ebp, start_eip);
      #ifdef USE_ELF_SYM
        if((sym = elf_find_sym(start_eip)))
        {
            kprintf(" <%s + 0x%x>", elf_get_str(sym->st_name), start_eip - sym->st_value);
        }
      #endif
        kprintf("\n");

        stack_frame = (struct stack_frame*) start_ebp;
    }
    else
    {
        __asm volatile ("mov %%ebp, %0" : "=r"(stack_frame));
    }

    for( ; stack_frame != 0 && stack_frame->ebp != 0; stack_frame = stack_frame->ebp)
    {
        kprintf("ebp %8p eip %8x", stack_frame->ebp, stack_frame->eip);

      #ifdef USE_ELF_SYM
        if((sym = elf_find_sym(stack_frame->eip)))
        {
            kprintf(" <%s + 0x%x>", elf_get_str(sym->st_name), stack_frame->eip - sym->st_value);
        }
      #endif

        if (!mmc_resolve(&mmc_current_context(), stack_frame->ebp)) {
            kprintf(" (nicht gemappt)\n");
            break;
        }

        kprintf("\n");
    }
}

void print_tasks_backtrace(void)
{
    pm_process_t* process;
    pm_thread_t* thread;
    machine_state_t* isf;
    int i, j;

    for (i = 0; (process = list_get_element_at(process_list, i)); i++)
    {
        kprintf("\n\nPID %d: %s\n"
            "Status %d, blocked_by_pid %d, blocked_count %d\n",
            process->pid,

            process->cmdline != NULL
                ? process->cmdline
                : "Unbekannter Task",

            process->status, process->blocked_by_pid, process->blocked_count);

        mmc_activate(&process->context);

        for (j = 0; (thread = list_get_element_at(process->threads, j)); j++) {
            kprintf("Thread %d: Status %d\n", j, thread->status);
            isf = (machine_state_t*) thread->kernel_stack;
            stack_backtrace(isf->ebp, isf->eip);
        }
    }

    mmc_activate(&current_process->context);
}
