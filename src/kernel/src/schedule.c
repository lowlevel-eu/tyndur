#include "types.h"
#include "kernel.h"
#include "intr.h"
#include "paging.h"
#include "kprintf.h"
  
 
#define USER_STACK_VIRT 0xfffff000
#define USER_STACK_SIZE (1024 - 4)
 

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
    dword * kernel_stack = ((dword*)phys_alloc_page());
    dword * user_stack = ((dword*)phys_alloc_page());

    map_page(kernel_page_directory, kernel_stack, kernel_stack, PTE_P | PTE_W | PTE_U);
    map_page(kernel_page_directory, user_stack, user_stack, PTE_P | PTE_W | PTE_U);

    kprintf("Neuer Task; KStack = 0x%x, UStack = 0x%x", kernel_stack, user_stack);

    kernel_stack += 1024;
//    user_stack += 1024;

    tasks[num_tasks].cr3 = (dword)phys_alloc_page();
    kprintf(", PD = 0x%x\n", tasks[num_tasks].cr3);
    kernel_identity_map((paddr_t)(tasks[num_tasks].cr3), PAGE_SIZE);
    map_page((page_directory_t)tasks[num_tasks].cr3, (vaddr_t)USER_STACK_VIRT, (paddr_t)user_stack, PTE_W | PTE_P | PTE_U);

    tasks[num_tasks].kernel_stack = (dword)kernel_stack;

    *(--kernel_stack) = 0x23; // ss
    *(--kernel_stack) = USER_STACK_VIRT + USER_STACK_SIZE; // esp
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
