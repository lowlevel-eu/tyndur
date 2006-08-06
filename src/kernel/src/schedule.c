#include "types.h"
#include "kernel.h"
#include "intr.h"

/* TODO: rewrite */

struct task tasks[MAX_TASKS];
int num_tasks;
int current_task_num;
int k = 3;
static void timer_irq(int irq)
{
    current_task_num++;
    if(current_task_num >= num_tasks)
    {
        current_task_num = 0;
    }

    current_task = &tasks[current_task_num];
}

void init_scheduler(void)
{
    request_irq(0, timer_irq);

    num_tasks = 1;

    current_task_num = 0;
    tasks[current_task_num].type = 0;
    current_task = &tasks[current_task_num];
}

int create_task(void * entry)
{
    dword * kernel_stack = ((dword*)phys_alloc_page()) + 1024;
    dword * user_stack = ((dword*)phys_alloc_page()) + 1024;

    tasks[num_tasks].kernel_stack = (dword)kernel_stack;

    *(--kernel_stack) = 0x23; // ss
    *(--kernel_stack) = (dword)user_stack; // esp
    *(--kernel_stack) = 0x3202; // eflags = interrupts aktiviert und iopl = 3
    *(--kernel_stack) = 0x1b; // cs
    *(--kernel_stack) = (dword)entry; // eip

    *(--kernel_stack) = 0; // interrupt nummer
    *(--kernel_stack) = 0; // error code

    // general purpose registers
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;

    // segment registers
    *(--kernel_stack) = 0x23;
    *(--kernel_stack) = 0x23;
    *(--kernel_stack) = 0x23;
    *(--kernel_stack) = 0x23;

    tasks[num_tasks].esp = (dword)kernel_stack;
    num_tasks++;

    return 0;
}
