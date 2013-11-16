#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stdbool.h>

#include "intr.h"
#include "syscallno.h"
#include <lost/config.h>

#ifdef CONFIG_DEBUG_LAST_SYSCALL
#define DEBUG_LAST_SYSCALL_DATA_SIZE 4
extern uint32_t debug_last_syscall_no;
extern pid_t debug_last_syscall_pid;
extern uint32_t debug_last_syscall_data[DEBUG_LAST_SYSCALL_DATA_SIZE];
#endif

void syscall(struct int_stack_frame ** esp);

bool syscall_rpc(uint32_t callee_pid, uint32_t data_size, char* data);

#endif
