#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "syscallno.h"

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
dma_mem_ptr_t mem_dma_allocate(dword size, dword flags);
void mem_free(void* address, dword size);
memory_info_t memory_info();

void yield();
void p();
void v();

void set_rpc_handler(void (*rpc_handler)());
void add_intr_handler(dword intr);

void rpc(pid_t pid);
void signal(pid_t pid, dword signal);
void send_message(pid_t pid, dword function, dword correlation_id, dword len, char* data);


pid_t get_pid();
pid_t create_process(dword initial_eip, uid_t uid, const char* path);
void destroy_process();
void init_child_page (pid_t pid, void* dest, void* src, size_t size);
void unblock_process(pid_t pid);
char* get_cmdline();

qword get_tick_count();

#endif
