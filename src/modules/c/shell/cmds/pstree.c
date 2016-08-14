/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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
#include <types.h>
#include "stdlib.h"
#include "stdio.h"
#include "syscall.h"
#include "unistd.h"
#include <lost/config.h>
#include <tms.h>

void pstree_display_usage(void);
void pstree_show_children(pid_t pid, uint8_t depth);
task_info_t* task_info;

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_pstree(int argc, char* argv[])
#else
    int main(int argc, char* argv[])
#endif
{
    if (argc > 1) {
        pstree_display_usage();
        return -1;
    }
    
    task_info = enumerate_tasks();
    pstree_show_children(0, 0);
    mem_free(task_info, task_info->info_size);
    
    return EXIT_SUCCESS;
}

void pstree_display_usage()
{
    printf(TMS(pstree_usage, "\nAufruf: pstree\n"));
}

void pstree_show_children(pid_t pid, uint8_t depth)
{
    uint32_t i, j;

    for (i = 0; i < task_info->task_count; i++) {
        if (task_info->tasks[i].parent_pid == pid) {
            for (j = 0; j < depth; j++) {
                printf("   ");
            }

            printf("%s\n", task_info->tasks[i].cmdline);

            pstree_show_children(task_info->tasks[i].pid, depth + 1);
        }
    }
}
