#ifndef TASKS_H
#define TASKS_H

#define MAX_TASKS 32

struct task
{
    pid_t pid;
    dword esp;
    dword kernel_stack;
    dword type;
    vaddr_t rpc_handler;
    dword cr3;
    dword slices;
    dword blocked_by_pid;
    vaddr_t user_stack_bottom;
    
    struct task* next_task;
};

struct task * current_task;


struct task * get_task(pid_t pid);

struct task * create_task(void * entry);
void destroy_task(struct task* task);

pid_t generate_pid();
#endif
