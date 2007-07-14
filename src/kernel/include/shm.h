#ifndef SHM_H
#define SHN_H

#include "types.h"
#include "collections.h"
#include "tasks.h"

typedef struct shm_table_entry_t
{
    paddr_t *addresses;
    dword pagecount;
    dword id;
    dword usecount; //Anzahl der angefügten Prozesse
} shm_table_entry_t;

//Tabelle aller shm-Bereiche
extern list_t *shm_table;

void init_shared_memory(void);
dword create_shared_memory(dword size);
vaddr_t attach_task_to_shm(struct task *task, dword id);
void detach_task_from_shm(struct task *task, dword id);

#endif
