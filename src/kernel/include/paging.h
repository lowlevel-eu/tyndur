#ifndef PAGING_H
#define PAGING_H

#include "vmm.h"

extern page_directory_t kernel_page_directory;

bool map_page_range(page_directory_t page_directory, vaddr_t vaddr, paddr_t paddr, int flags, int num_pages);
bool map_page(page_directory_t page_directory, vaddr_t vaddr, paddr_t paddr, int flags);
bool unmap_page(page_directory_t page_directory, vaddr_t vaddr);
paddr_t resolve_vaddr(page_directory_t page_directory, vaddr_t vaddr);

extern bool kernel_identity_map(paddr_t paddr, dword bytes);

#endif
