#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "syscall.h"
#include "unistd.h"
#include "config.h"

void ps_display_usage();

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_ps(int argc, char* argv[], const char* args)
#else
    #define _USE_START_
    #include "init.h"
    int main(int argc, char* argv[])
#endif
{
    if (argc > 1) {
        ps_display_usage();
        return -1;
    }

    task_info_t* task_info = enumerate_tasks();
    dword i;
    
    puts(" PID  PPID Status CMD");
    for (i = 0; i < task_info->task_count; i++) {
        printf("  %02d  %02d  %01d  %s\n", task_info->tasks[i].pid, 
            task_info->tasks[i].parent_pid, task_info->tasks[i].status,
            task_info->tasks[i].cmdline);
    }

    return EXIT_SUCCESS;
}

void ps_display_usage()
{
    puts("\nAufruf: ps");
}

