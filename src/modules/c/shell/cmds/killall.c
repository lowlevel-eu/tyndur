/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <lost/config.h>

#include <errno.h>
#include <signal.h>
#include <tms.h>

static void killall_display_usage(void);

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_killall(int argc, char* argv[])
#else
    int main(int argc, char* argv[])
#endif
{
    int ret=0,i=0,fail=0,matches=0;
    task_info_t *task_info = NULL;
    char *process;
    char copied_cmdline[255];

    if (argc < 2) {
        killall_display_usage();
        return -1;
    }

    task_info = enumerate_tasks();

    if (task_info == NULL) {
        return EXIT_FAILURE;
    }


    for (i = 0; i < task_info->task_count; ++i) {
        strncpy(copied_cmdline, task_info->tasks[i].cmdline, 255);

        process = strtok((char *) task_info->tasks[i].cmdline, " ");
        process = process == NULL
                ? strrchr(task_info->tasks[i].cmdline, '/')
                : strrchr(process, '/');
        ++process;

        if (strcmp(argv[1], process) == 0) {
            ++matches;
            ret = kill(task_info->tasks[i].pid, SIGTERM);

            if (ret < 0) {
                printf(TMS(killall_cant_kill,
                        "Konnte pid %d - %s nicht beenden: %s (%s)\n"),
                        task_info->tasks[i].pid, errno, strerror(errno));

                if (!fail) {
                    fail = 1;
                }
            } else {
                printf(TMS(killall_killed, "pid %d beendet\n"),
                       task_info->tasks[i].pid);
            }
        }

        process = NULL;
    }

    mem_free(task_info, task_info->info_size);

    if (matches == 0) {
        printf(TMS(killall_not_found, "Kein passender Prozess gefunden\n"));
    }

    if (fail) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static void killall_display_usage()
{
    printf(TMS(killall_usage, "Aufruf: killall <name>\n"));
}

