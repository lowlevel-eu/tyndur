#ifndef _PHYS_H_
#define _PHYS_H_

#include <types.h>

void phys_mark_page_as_free(paddr_t page);
unsigned long phys_count_free_pages(void);
unsigned long phys_count_pages(void);

paddr_t phys_alloc_dma_page_range_64k(unsigned int num);

#endif

