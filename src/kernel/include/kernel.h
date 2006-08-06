#ifndef KERNEL_H
#define KERNEL_H

void panic(char * message, ...);

struct task
{
    dword esp;
    dword kernel_stack;
    dword type;
    dword cr3;
    dword slices;
};

#define MAX_TASKS 32

struct task * current_task;

int create_task(void * entry);

void * phys_alloc_page();
void * phys_alloc_dma_page();
void phys_free_page(void * page);

#endif /* ndef KERNEL_H */
