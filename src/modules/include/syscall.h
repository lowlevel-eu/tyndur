#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "syscallno.h"

int puts(const char* s);
int syscall_puts(const char* s);

void* mem_allocate(dword size, dword flags);

void yield();
void p();
void v();

void set_rpc_handler(void (*rpc_handler)());
void add_intr_handler(dword intr);

void rpc(dword pid);
void signal(dword pid, dword signal);
void send_message(dword pid, dword function, dword correlation_id, dword len, char* data);


#endif
