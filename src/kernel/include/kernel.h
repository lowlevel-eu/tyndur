#ifndef KERNEL_H
#define KERNEL_H

#include "types.h"

__attribute__((noreturn)) void panic(char * message, ...);

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
void * phys_alloc_page_range(unsigned int num);
void phys_free_page(void * page);
void phys_free_page_range(void * page, unsigned int num);

extern void kernel_start();
extern void kernel_phys_start();
extern void kernel_end();
extern void kernel_phys_end();

#endif /* ndef KERNEL_H */
