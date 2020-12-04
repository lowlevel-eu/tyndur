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

#include <lost/config.h>
#include "kprintf.h"
#include "multiboot.h"
#include "debug.h"

void stack_backtrace(uintptr_t start_rbp, uintptr_t start_rip)
{
    struct stack_frame
    {
        struct stack_frame * rbp;
        uintptr_t rip;
       /* dword args[0];*/
    } * stack_frame;
    
    kprintf("stack backtrace:\n");

    if (start_rbp != 0)
    {
        kprintf("rbp %16x eip %16x", start_rbp, start_rip);
        /*if((sym = elf_find_sym(start_eip)))
        {
            kprintf(" <%s + 0x%x>", elf_get_str(sym->st_name), start_eip - sym->st_value);
        }*/
        kprintf("\n");

        stack_frame = (struct stack_frame*) start_rbp;
    }
    else
    {
        __asm volatile ("mov %%rbp, %0" : "=r"(stack_frame));
    }

    for( ; stack_frame != 0 && stack_frame->rbp != 0; stack_frame = stack_frame->rbp)
    {
        kprintf("rbp %16x rip %16x", stack_frame->rbp, stack_frame->rip);
        /*
        if((sym = elf_find_sym(stack_frame->eip)))
        {
            kprintf(" <%s + 0x%x>", elf_get_str(sym->st_name), stack_frame->eip - sym->st_value);
        }
        */
        kprintf("\n");
    }
}

