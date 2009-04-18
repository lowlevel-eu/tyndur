/*
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "syscallno.h"
#include "syscall_structs.h"
typedef struct 
{
    vaddr_t virt;
    paddr_t phys;
} dma_mem_ptr_t;

typedef struct
{
    dword total;
    dword free;
} memory_info_t;

//int puts(const char* s);
int syscall_putsn(unsigned int n, const char* s);


void* mem_allocate(dword size, dword flags);
void *mem_allocate_physical(dword size, dword position, dword flags);
dma_mem_ptr_t mem_dma_allocate(dword size, dword flags);
bool mem_free(void* address, dword size);
void mem_free_physical(void* address, dword size);
memory_info_t memory_info(void);
void* get_phys_addr(void* address);

dword create_shared_memory(dword size);
void *open_shared_memory(dword id);
void close_shared_memory(dword id);

bool request_ports(dword port, dword length);
bool release_ports(dword port, dword length);

void yield(void);
void wait_for_rpc(void);
void p(void);
void v(void);

void set_rpc_handler(void (*rpc_handler)(void));
void add_intr_handler(dword intr);

void rpc(pid_t pid);
int send_message(pid_t pid, dword function, dword correlation_id,
    dword len, char* data);
void wait_for_rpc(void);
void v_and_wait_for_rpc(void);

pid_t get_pid(void);
pid_t get_parent_pid(pid_t pid);
pid_t create_process(dword initial_eip, uid_t uid, const char* path, pid_t parent_pid);
void destroy_process(void);
void init_child_page (pid_t pid, void* dest, void* src, size_t size);
void init_child_page_copy (pid_t pid, void* dest, void* src, size_t size);
void unblock_process(pid_t pid);
char* get_cmdline(void);

bool vm86_int(vm86_regs_t *regs, dword *shm);

task_info_t* enumerate_tasks(void);

qword get_tick_count(void);

void syscall_timer(dword timer_id, dword usec);

void syscall_debug_stacktrace(pid_t pid);

#endif
