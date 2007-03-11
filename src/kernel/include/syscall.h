#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "intr.h"
#include "syscallno.h"
#include "config.h"

#ifdef CONFIG_DEBUG_LAST_SYSCALL
#define DEBUG_LAST_SYSCALL_DATA_SIZE 4
extern dword debug_last_syscall_no;
extern dword debug_last_syscall_data[DEBUG_LAST_SYSCALL_DATA_SIZE];
#endif

void syscall(struct int_stack_frame ** esp);

void syscall_rpc(dword callee_pid, dword data_size, char* data);

#endif
