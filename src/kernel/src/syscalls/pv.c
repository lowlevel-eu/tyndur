/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
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

#include <types.h>
#include <stdint.h>
#include <string.h>

#include "syscall.h"
#include "tasks.h"
#include "cpu.h"

/**
 * Kritischen Abschnitt einleiten (Keine RPCs ausser Antworten sind mehr
 * zulaessig)
 */
int syscall_pm_p(void)
{
    pm_block_rpc(current_process, current_process->pid);

    return 0;
}

/**
 * Kritischen Abschnitt verlassen
 *
 * @param pid PID, fuer die wieder RPCs zugelassen werden sollen. 0 fuer
 * den aufrufenden Prozess selbst
 */
int syscall_pm_v(pid_t pid)
{
    if (pid == 0) {
        pm_unblock_rpc(current_process, current_process->pid);
    } else {
        pm_process_t* process = pm_get(pid);
        if (process == NULL) {
            return -1;
        }
        pm_unblock(process);
        pm_unblock_rpc(process, current_process->pid);
    }

    return 0;
}

/**
 * Kritischen Abschnitt verlassen und auf RPC warten
 *
 * @param pid PID, fuer die wieder RPCs zugelassen werden sollen. 0 fuer
 * den aufrufenden Prozess selbst
 */
int syscall_pm_v_and_wait_for_rpc()
{
    syscall_pm_v(0);
    syscall_pm_wait_for_rpc();

    return 0;
}
