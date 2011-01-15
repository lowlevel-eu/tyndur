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

#include <stdint.h>
#include <stdbool.h>

#include "elf32.h"
#include "kprintf.h"
#include "multiboot.h"
#include "paging.h"
#include "debug.h"
#include "string.h"
#include "intr.h"
#include "tasks.h"

static Elf32_Shdr * symtab = 0; // Zeiger auf den ELF-Header-Eintrag der Symbol Tabelle ...
static Elf32_Shdr * strtab = 0; // ... und der String Tabelle

static uint32_t debug_flags = 0;

/**
 * Gibt einen Zeiger auf das Symbol mit der größten Adresse kleiner als addr 
 * zurück. Wenn kein Symbol bestimmt werden kann, wird 0 zurück gegeben.
 */
static Elf32_Sym * elf_find_sym(uint32_t addr)
{
    Elf32_Sym * sym;
    int i;

    if(symtab == 0)
    {
        return 0;
    }

    sym = (Elf32_Sym *)symtab->sh_addr;

    for(i = 0; i < symtab->sh_size / sizeof(Elf32_Sym); i++)
    {
        if(addr >= (uint32_t)sym[i].st_value && addr < (uint32_t)sym[i].st_value + sym[i].st_size)
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
    if(strtab == 0)
    {
        return "";
    }

    return (char*)strtab->sh_addr + index;
}

/**
 * Mappt den ELF-Header, sowie die ELF-Symbol-Tabelle und die dazugehörige
 * String-Tabelle, wenn sie vorhanden sind.
 */
void init_stack_backtrace(void)
{
    Elf32_Shdr * elfshdr = (Elf32_Shdr *)multiboot_info.mi_elfshdr_addr;
    int i;

    /* ELF Header mappen */
    map_page_range(kernel_page_directory, elfshdr, (paddr_t) elfshdr,
        PTE_W | PTE_P /*| PTE_U*/,
        NUM_PAGES(multiboot_info.mi_elfshdr_num * sizeof(Elf32_Shdr)));

    if(multiboot_info.mi_flags & MULTIBOOT_INFO_HAS_ELF_SYMS)
    {
        /* Symbol Table und String Table suchen */
        for (i = 0; i < multiboot_info.mi_elfshdr_num; i++)
        {
            if(elfshdr[i].sh_type == SHT_SYMTAB)
            {
                symtab = &elfshdr[i];
                strtab = &elfshdr[symtab->sh_link];
                
                break;
            }
        }
    }

    if(symtab != 0 && strtab != 0)
    {
        /* Symbol Tabelle und String Tabelle mappen */

        kernel_identity_map((paddr_t)symtab->sh_addr, symtab->sh_size);
        kernel_identity_map((paddr_t)strtab->sh_addr, strtab->sh_size);
    }
}

void stack_backtrace_ebp(uint32_t start_ebp, uint32_t start_eip)
{
    struct stack_frame
    {
        struct stack_frame * ebp;
        uint32_t eip;
       /* uint32_t args[0];*/
    } * stack_frame;
    Elf32_Sym * sym;

    kprintf("stack backtrace:\n");

    if (start_ebp != 0)
    {
        kprintf("ebp %08x eip %08x", start_ebp, start_eip);
        if((sym = elf_find_sym(start_eip)))
        {
            kprintf(" <%s + 0x%x>", elf_get_str(sym->st_name), start_eip - sym->st_value);
        }
        kprintf("\n");

        stack_frame = (struct stack_frame*) start_ebp;
    }
    else
    {
        __asm volatile ("mov %%ebp, %0" : "=r"(stack_frame));
    }

    for( ; is_userspace(stack_frame, sizeof(*stack_frame)) && stack_frame->ebp != 0; stack_frame = stack_frame->ebp)
    {
        kprintf("ebp %08x eip %08x", stack_frame->ebp, stack_frame->eip);

        if((sym = elf_find_sym(stack_frame->eip)))
        {
            kprintf(" <%s + 0x%x>", elf_get_str(sym->st_name), stack_frame->eip - sym->st_value);
        }

        kprintf("\n");
    }

    if (stack_frame && !is_userspace(stack_frame, sizeof(*stack_frame))) {
        kprintf("Stack kaputt.\n");
    }
}

void stack_backtrace(void)
{
    stack_backtrace_ebp(0, 0);
}

///Setzt die richtigen Debug-Flags anhand der Commandline vom bootloader
void debug_parse_cmdline(char* cmdline)
{
    char* pos = strstr(cmdline, "debug=");
    debug_flags = 0;

    if(pos == NULL) return;
    
    //Debug= ueberspringen
    pos += 6;
    while((*pos != 0) && (*pos != ' '))
    {
        switch(*pos)
        {
            case 'i':
                debug_flags |= DEBUG_FLAG_INIT;
                break;

            case 'p':
                debug_flags |= DEBUG_FLAG_PEDANTIC;
                break;

            case 's':
                debug_flags |= DEBUG_FLAG_STACK_BACKTRACE;
                break;

            case 'c':
                debug_flags |= DEBUG_FLAG_SYSCALL;
                break;

            case 'n':
                debug_flags |= DEBUG_FLAG_NO_KCONSOLE;
                break;
        }
        pos++;
    }
}

///Ueberprueft ob ein bestimmtes Debug-Flag gesetzt ist
bool debug_test_flag(uint32_t flag)
{
    return ((debug_flags & flag) != 0);
}

///Gibt die Debug-Meldung aus, wenn das Flag gesetzt ist
void debug_print(uint32_t flag, const char* message)
{
    if(debug_test_flag(flag))
    {
        kprintf("DEBUG: %s\n", message);
    }
}


void print_tasks_backtrace(void)
{
    struct task* task = first_task;
    struct int_stack_frame* isf;

    while (task != NULL)
    {
        kprintf("\n\nPID %d: %s\n"
            "Status %d, blocked_by_pid %d, blocked_count %d\n",
            task->pid, 

            (task == 0 
                ? "Kernel-Initialisierung"
                : (task->cmdline != NULL 
                    ? task->cmdline 
                    : "Unbekannter Task")),

            task->status, task->blocked_by_pid, task->blocked_count);
        
        __asm("mov %0, %%cr3" : : "r"(resolve_vaddr(kernel_page_directory, task->cr3)));
        
        isf = (struct int_stack_frame*) task->esp;
        stack_backtrace_ebp(isf->ebp, isf->eip);

        task = task->next_task;
    }
        
    __asm("mov %0, %%cr3" : : "r"(resolve_vaddr(kernel_page_directory, current_task->cr3)));
}
