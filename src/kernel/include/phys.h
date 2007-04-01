#ifndef _PHYS_H_
#define _PHYS_H_

void phys_mark_page_as_free(void * page);
unsigned long phys_count_free_pages();
unsigned long phys_count_pages();

#endif

