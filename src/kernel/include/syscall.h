#ifndef SYSCALL_H
#define SYSCALL_H

#include "intr.h"

#define SYSCALL_KPRINTF 0

#define SYSCALL_MEM_ALLOCATE 1
#define SYSCALL_MEM_FREE 2

#define SYSCALL_PM_CREATE_PROCESS 3
#define SYSCALL_PM_EXIT_PROCESS 5
#define SYSCALL_PM_SLEEP 6
#define SYSCALL_PM_SETUID 7
#define SYSCALL_PM_REQUEST_PORT 8
#define SYSCALL_PM_RELEASE_PORT 9

#define SYSCALL_SET_RPC_HANDLER 50
#define SYSCALL_RPC 51
#define SYSCALL_ADD_INTERRUPT_HANDLER 52

void syscall(struct int_stack_frame ** esp);

void syscall_rpc(dword callee_pid, dword data_size, char* data);

#endif
