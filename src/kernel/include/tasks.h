#ifndef TASKS_H
#define TASKS_H

#include <types.h>
#include <stdint.h>
#include <stdbool.h>

#include "vmm.h"
#include "collections.h"
#include "syscall_structs.h"

#define TS_RUNNING 0
#define TS_WAIT_FOR_RPC 1

typedef struct
{
    uint32_t *meminfo;
    vm86_regs_t *regs;
} vm86_info_t;

struct task
{
    pid_t pid;
    uint32_t esp;
    uint32_t kernel_stack;
    uint32_t type;
    vaddr_t rpc_handler;
    list_t* rpcs;
    page_directory_t cr3;
    uint32_t pd_id;
    uint32_t slices;
    uint32_t blocked_by_pid;
    uint32_t blocked_count;
    vaddr_t user_stack_bottom;
    uint32_t status;
    
    struct task* parent_task;

    const char* cmdline;
    
    void* io_bitmap;

    struct task* next_task;

//Scheduling-Infos
    ///Das Maximum der Zeit, die der Task die CPU benutzen darf
    uint8_t schedule_ticks_max;
    ///Verbleibende Zeit
    int schedule_ticks_left;
    
    //Shared Memory
    list_t *shmids;
    list_t *shmaddresses;

    // Benutzter speicher
    uint32_t memory_used;
    
    // VM86-Info
    bool vm86;
    vm86_info_t *vm86_info;
};

extern struct task* current_task;
extern struct task* first_task;

void schedule(uint32_t* esp);
void schedule_to_task(struct task* target_task, uint32_t* esp);
void set_io_bitmap(void);

// TODO: Nur wegen vm86.c hier
pid_t generate_pid(void);
struct task * create_task(void * entry, const char* cmdline, pid_t parent);
void destroy_task(struct task* task);
struct task * get_task(pid_t pid);


bool block_task(struct task* task, pid_t blocked_by);
bool unblock_task(struct task* task, pid_t blocked_by);

void abort_task(char* format, ...);

#endif
