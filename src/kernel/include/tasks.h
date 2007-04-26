#ifndef TASKS_H
#define TASKS_H

#include "types.h"
#include "vmm.h"
#include "collections.h"

struct task
{
    pid_t pid;
    dword esp;
    dword kernel_stack;
    dword type;
    vaddr_t rpc_handler;
    page_directory_t cr3;
    dword slices;
    dword blocked_by_pid;
    dword blocked_count;
    vaddr_t user_stack_bottom;
    
    const char* cmdline;
    
    ///Nur ein Workaround um die PFs bei kaputten EFLAGS nach RPC zu verhindern
    list_t* eflags_stack;

    struct task* next_task;
};

struct task * current_task;


void schedule(dword* esp);

pid_t generate_pid();
struct task * create_task(void * entry, const char* cmdline);
void destroy_task(struct task* task);
struct task * get_task(pid_t pid);


bool block_task(struct task* task, pid_t blocked_by);
bool unblock_task(struct task* task, pid_t blocked_by);

void abort_task(char* format, ...);

#endif
