/*  
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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

#include "types.h"
#include "tasks.h"
#include "kprintf.h"

extern qword timer_ticks;

/**
 * Bricht den aktuell ausgeführten Task mit einer Fehlermeldung ab.
 */
void abort_task(char* format, ...)
{
    int * args = ((int*)&format) + 1;

    kprintf("\n"
      "\033[1;37m\033[41m" // weiss auf rot
      "Task %d beendet: ", current_task->pid);
    kaprintf(format, &args);
    kprintf("\n\033[0;37m\033[40m");

    destroy_task(current_task);
    while(1) {
        asm("sti; hlt");
    }
}

/**
 * Blockiert einen Task. Zu einem blockierten Task können keine RPC-Aufrufe
 * durchgeführt werden. Wenn der Task durch einen anderen Task blockiert ist,
 * wird er außerdem vom Scheduler nicht mehr aufgerufen, bis der Task 
 * entblockt wird.
 *
 * @param task Zu blockierender Task
 * @param blocked_by PID des blockierenden Tasks
 */
void block_task(struct task* task, pid_t blocked_by)
{
    //unsigned long timeout = timer_ticks + 100;
    
    if(task->blocked_by_pid == blocked_by) {
        task->blocked_count++;
        return;
    }

    while(task->blocked_by_pid)
    {
        asm("int $0x20");
    }
    
    task->blocked_by_pid = blocked_by;
    task->blocked_count = 1;
}

/**
 * Entblockt einen blockierten Task. Dies ist nur möglich, wenn der
 * aufrufende Task dem blockierenden Task entspricht
 *
 * @param task Zu entblockender Task
 * @param blocked_by Aufrufender Task
 *
 * @return true, wenn der Task erfolgreich entblockt wurde, false
 * im Fehlerfall.
 */
bool unblock_task(struct task* task, pid_t blocked_by)
{
    if (task->blocked_by_pid == blocked_by) {
        if(--(task->blocked_count) == 0) {
            task->blocked_by_pid = 0;
        }
        return TRUE;
    } else {
        return FALSE;
    }
}
