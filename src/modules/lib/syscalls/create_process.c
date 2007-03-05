#include "syscall.h"

pid_t get_pid()
{
    pid_t pid;
    asm(
        "mov %1, %%eax;"
        "int $0x30;"
        : "=a" (pid) : "i" (SYSCALL_PM_GET_PID));
    
    return pid;
}

pid_t create_process(dword initial_eip, uid_t uid)
{
    pid_t pid;

    asm(
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x8, %%esp;"
        : "=a" (pid) : "i" (SYSCALL_PM_CREATE_PROCESS), "r" (initial_eip), "r" (uid));
    return pid;
}

void destroy_process()
{
    asm(    "mov %0, %%eax;"
            "int $0x30;"
        : : "i" (SYSCALL_PM_EXIT_PROCESS));
}

