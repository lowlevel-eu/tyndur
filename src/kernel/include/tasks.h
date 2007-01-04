#ifndef TASKS_H
#define TASKS_H

#define MAX_TASKS 32

struct task
{
    dword esp;
    dword kernel_stack;
    dword type;
    vaddr_t rpc_handler;
    dword cr3;
    dword slices;
    dword blocked_by_pid;
	vaddr_t user_stack_bottom;
};

struct task * current_task;


struct task * get_task(pid_t pid);
pid_t get_pid(struct task * task);

struct task * create_task(void * entry);

#endif
