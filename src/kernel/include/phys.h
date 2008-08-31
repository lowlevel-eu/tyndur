#ifndef _PHYS_H_
#define _PHYS_H_

void phys_mark_page_as_free(void * page);
unsigned long phys_count_free_pages(void);
unsigned long phys_count_pages(void);

void * phys_alloc_dma_page_range_64k(unsigned int num);

#endif

