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
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"
#include "unistd.h"
#include <lost/config.h>
#include <lostio.h>
#include <tms.h>

void ps_display_usage(void);

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_ps(int argc, char* argv[])
#else
    int main(int argc, char* argv[])
#endif
{
    bool with_eip;
    char* temp;
    char* process;
    char copied_cmdline[255];
    bool color = file_is_terminal(stdout);

    if (argc > 2) {
        ps_display_usage();
        return -1;
    }

    if (argc > 1) {
        if (strcmp(argv[1], "--with-eip") == 0) {
            with_eip = true;
        } else {
            with_eip = false;
        }
    } else {
        with_eip = false;
    }

    task_info_t* task_info = enumerate_tasks();
    uint32_t i;

    if (color) {
        printf("\033[1m");
    }
    if (with_eip) {
        printf(TMS(ps_with_eip,
            " PID  PPID Status  EIP      Speicher  CMD\n"));
    } else {
        printf(TMS(ps_without_eip, " PID  PPID Status Speicher  CMD\n"));
    }
    if (color) {
        printf("\033[0m");
    }

    for (i = 0; i < task_info->task_count; i++) {
        strncpy(copied_cmdline, task_info->tasks[i].cmdline, 255);

        process = strtok((char*)task_info->tasks[i].cmdline, " ");
        process = strrchr(process, '/');

        if ((temp = strchr(copied_cmdline, ' '))) {
            strcat(process, temp);
        }

        if (with_eip) {
            printf("%4d  %4d     %2d  %08x  %6ik  %s\n", task_info->tasks[i].pid,
                task_info->tasks[i].parent_pid, task_info->tasks[i].status,
                task_info->tasks[i].eip,
                (task_info->tasks[i].memory_used / 1000),
                ++process);
        } else {
            printf("%4d  %4d     %2d  %6ik  %s\n", task_info->tasks[i].pid,
                task_info->tasks[i].parent_pid, task_info->tasks[i].status,
                (task_info->tasks[i].memory_used / 1000),
                ++process);
        }

        process = NULL;
        temp = NULL;
    }

    mem_free(task_info, task_info->info_size);

    return EXIT_SUCCESS;
}

void ps_display_usage()
{
    printf(TMS(ps_usage, "\nAufruf: ps [--with-eip]\n"));
}

