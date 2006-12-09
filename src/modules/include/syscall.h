#ifndef SYSCALL_H
#define SYSCALL_H

int puts(const char* s);

void set_rpc_handler(void* rpc_handler);
void add_intr_handler(dword intr);

#endif
