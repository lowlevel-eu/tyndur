#ifndef _RPC_H_
#define _RPC_H_

#include <stdint.h>
#include <stdbool.h>

#include "tasks.h"
#include "intr.h"

bool fastrpc(struct task * callee, uint32_t metadata_size, void* metadata, uint32_t data_size, void* data);
bool fastrpc_irq(struct task * callee, uint32_t metadata_size, void* metadata, 
    uint32_t data_size, void* data, uint8_t irq);

void return_from_rpc(struct int_stack_frame ** esp);
void rpc_destroy_task_backlinks(struct task* destroyed_task);

#endif
