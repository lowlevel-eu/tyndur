#ifndef PAGING_H
#define PAGING_H

#include <types.h>
#include <stdint.h>

#include "vmm.h"
#include "tasks.h"

extern page_directory_t kernel_page_directory;

bool map_page_range(page_directory_t page_directory, vaddr_t vaddr, paddr_t paddr, int flags, int num_pages);
bool map_page(page_directory_t page_directory, vaddr_t vaddr, paddr_t paddr, int flags);
bool unmap_page(page_directory_t page_directory, vaddr_t vaddr);
paddr_t resolve_vaddr(page_directory_t page_directory, vaddr_t vaddr);
bool is_userspace(vaddr_t start, uint32_t len);

vaddr_t find_contiguous_pages(page_directory_t page_directory, int num,
    uint32_t lower_limit, uint32_t upper_limit);
void increase_user_stack_size(struct task * task_ptr, int pages);

extern bool kernel_identity_map(paddr_t paddr, uint32_t bytes);

#endif
