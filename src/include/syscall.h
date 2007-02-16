#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "syscallno.h"

int puts(const char* s);
int syscall_puts(const char* s);

void* mem_allocate(dword size, dword flags);

void yield();

void set_rpc_handler(void (*rpc_handler)());
void add_intr_handler(dword intr);

void rpc(pid_t pid);
void signal(pid_t pid, dword signal);
void send_message(pid_t pid, dword function, dword correlation_id, dword len, char* data);


pid_t create_process(dword initial_eip, uid_t uid);
void init_child_page (pid_t pid, void* dest, void* src, size_t size);
void unblock_process(pid_t pid);


#endif
