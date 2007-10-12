/*
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Antoine Kaufmann.
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

#include <types.h>
#include <stdint.h>
#include <string.h>

#include "syscall.h"
#include "tasks.h"
#include "cpu.h"

/**
 * PID des aktuellen Prozesses ausfindig machen
 *
 * @return PID
 */
pid_t syscall_pm_get_pid()
{
    return cpu_get_current()->thread->process->pid;
}

/**
 * PID des Elternprozesses ausfindig machen
 *
 * @param pid PID des Prozesses
 *
 * @return PID
 */
pid_t syscall_pm_get_parent_pid(pid_t pid)
{
    pid_t parent_pid = 0;
    return parent_pid;
}

/**
 * Befehlszeile des aktuellen Prozesses abfragen
 *
 * @return Pointer auf die Befehlszeile
 */
const char* syscall_pm_get_cmdline()
{
    const char* cmdline = cpu_get_current()->thread->process->cmdline;
    if (cpu_get_current()->thread->process->cmdline == NULL) {
        return NULL;
    }
    size_t cmdlinesize = strlen(cmdline) + 1;
    vaddr_t address = mmc_automap(&mmc_current_context(), pmm_alloc(NUM_PAGES(
        cmdlinesize)), NUM_PAGES(cmdlinesize), USER_MEM_START, USER_MEM_END,
        MM_FLAGS_USER_DATA);
    memcpy(address, cmdline, cmdlinesize);

    return (const char*) address;
}

