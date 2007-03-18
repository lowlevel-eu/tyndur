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

pid_t create_process(dword initial_eip, uid_t uid, const char* args)
{
    pid_t pid;

    asm(
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0xc, %%esp;"
        : "=a" (pid) : "i" (SYSCALL_PM_CREATE_PROCESS), "r" (initial_eip), "r" (uid), "r" (args));
    return pid;
}

void destroy_process()
{
    asm(    "mov %0, %%eax;"
            "int $0x30;"
        : : "i" (SYSCALL_PM_EXIT_PROCESS));
}

char* get_cmdline()
{
    char* result;
    asm(
        "mov %1, %%eax;"
        "int $0x30;"
        : "=a" (result) : "i" (SYSCALL_PM_GET_CMDLINE));

    return result;
}
