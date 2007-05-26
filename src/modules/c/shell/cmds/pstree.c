#include "stdlib.h"
#include "stdio.h"
#include "syscall.h"
#include "unistd.h"
#include "config.h"

void pstree_display_usage();
void pstree_show_children(pid_t pid, byte depth);
task_info_t* task_info;

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_pstree(int argc, char* argv[], const char* args)
#else
    #define _USE_START_
    #include "init.h"
    int main(int argc, char* argv[])
#endif
{
    if (argc > 1) {
        pstree_display_usage();
        return -1;
    }
    
    task_info = enumerate_tasks();
    pstree_show_children(0, 0);
    
    return EXIT_SUCCESS;
}

void pstree_display_usage()
{
    puts("\nAufruf: pstree");
}

void pstree_show_children(pid_t pid, byte depth)
{
    dword i, j;

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
