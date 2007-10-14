#ifndef _RPC_H_
#define _RPC_H_

#include "types.h"
#include "tasks.h"
#include "intr.h"

bool fastrpc(struct task * callee, dword metadata_size, void* metadata, dword data_size, void* data);
bool fastrpc_irq(struct task * callee, dword metadata_size, void* metadata, 
    dword data_size, void* data, byte irq);

void return_from_rpc(struct int_stack_frame ** esp);
void rpc_destroy_task_backlinks(struct task* destroyed_task);

#endif
