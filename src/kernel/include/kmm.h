#ifndef KMM_H
#define KMM_H

#include "types.h"

vaddr_t find_contiguous_kernel_pages(int num);
vaddr_t map_phys_addr(paddr_t paddr, size_t size);
void free_phys_addr(vaddr_t vaddr, size_t size);

#endif
