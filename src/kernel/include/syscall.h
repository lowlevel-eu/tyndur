#ifndef SYSCALL_H
#define SYSCALL_H

#include "intr.h"
#include "syscallno.h"

void syscall(struct int_stack_frame ** esp);

void syscall_rpc(dword callee_pid, dword data_size, char* data);

#endif
