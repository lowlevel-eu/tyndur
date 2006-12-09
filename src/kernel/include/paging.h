#ifndef PAGING_H
#define PAGING_H

#include "vmm.h"
#include "types.h"

extern page_directory_t kernel_page_directory;

bool map_page_range(page_directory_t page_directory, vaddr_t vaddr, paddr_t paddr, int flags, int num_pages);
bool map_page(page_directory_t page_directory, vaddr_t vaddr, paddr_t paddr, int flags);
bool unmap_page(page_directory_t page_directory, vaddr_t vaddr);
paddr_t resolve_vaddr(page_directory_t page_directory, vaddr_t vaddr);
vaddr_t find_contiguous_pages(page_directory_t page_directory, int num, dword lower_limit, dword upper_limit);

extern bool kernel_identity_map(paddr_t paddr, dword bytes);

#endif
