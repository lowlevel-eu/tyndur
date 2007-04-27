#ifndef _RPC_H_
#define _RPC_H_

#include "types.h"
#include "tasks.h"
#include "intr.h"

bool fastrpc(struct task * callee, dword metadata_size, void* metadata, dword data_size, void* data);
void return_from_rpc(struct int_stack_frame ** esp);

#endif
