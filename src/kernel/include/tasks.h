#ifndef TASKS_H
#define TASKS_H

#include "types.h"
#include "vmm.h"
#include "collections.h"


#define TS_RUNNING 0
#define TS_WAIT_FOR_RPC 1

struct task
{
    pid_t pid;
    dword esp;
    dword kernel_stack;
    dword type;
    vaddr_t rpc_handler;
    list_t* rpcs;
    page_directory_t cr3;
    dword slices;
    dword blocked_by_pid;
    dword blocked_count;
    vaddr_t user_stack_bottom;
    dword status;
    
    struct task* parent_task;

    const char* cmdline;
    
    void* io_bitmap;

    struct task* next_task;

//Scheduling-Infos
    ///Das Maximum der Zeit, die der Task die CPU benutzen darf
    byte schedule_ticks_max;
    ///Verbleibende Zeit
    int schedule_ticks_left;
};

extern struct task* current_task;
extern struct task* first_task;

void schedule(dword* esp);
void schedule_to_task(struct task* target_task, dword* esp);

struct task * create_task(void * entry, const char* cmdline, pid_t parent);
void destroy_task(struct task* task);
struct task * get_task(pid_t pid);


bool block_task(struct task* task, pid_t blocked_by);
bool unblock_task(struct task* task, pid_t blocked_by);

void abort_task(char* format, ...);

#endif
