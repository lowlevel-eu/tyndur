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
void mem_free(void* address, dword size);
void mem_free_physical(void* address, dword size);
memory_info_t memory_info();

dword create_shared_memory(dword size);
void *open_shared_memory(dword id);
void close_shared_memory(dword id);

bool request_ports(dword port, dword length);
bool release_ports(dword port, dword length);

void yield();
void wait_for_rpc();
void p();
void v();

void set_rpc_handler(void (*rpc_handler)());
void add_intr_handler(dword intr);

void rpc(pid_t pid);
void signal(pid_t pid, dword signal);
void send_message(pid_t pid, dword function, dword correlation_id, dword len, char* data);
void wait_for_rpc();
void v_and_wait_for_rpc();

pid_t get_pid();
pid_t get_parent_pid(pid_t pid);
pid_t create_process(dword initial_eip, uid_t uid, const char* path, pid_t parent_pid);
void destroy_process();
void init_child_page (pid_t pid, void* dest, void* src, size_t size);
void unblock_process(pid_t pid);
char* get_cmdline();

task_info_t* enumerate_tasks();

qword get_tick_count();

void syscall_timer(dword timer_id, dword usec);

#endif
