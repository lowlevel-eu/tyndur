/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Mathias Gottschlag.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */  

#include <stdint.h>

#include "shm.h"
#include "phys.h"
#include "stdlib.h"
#include "kernel.h"
#include "paging.h"

list_t *shm_table = 0;

uint32_t lastid = 0;

void init_shared_memory(void)
{
	shm_table = list_create();
}

uint32_t create_shared_memory(uint32_t size)
{
    shm_table_entry_t *entry = malloc(sizeof(shm_table_entry_t));
    
    entry->pagecount = size / PAGE_SIZE;
    if (size%PAGE_SIZE) {
        entry->pagecount++;
    }
    entry->addresses = malloc(entry->pagecount * sizeof(paddr_t));
    int i;
    for (i = 0; i < entry->pagecount; i++) {
        entry->addresses[i] = phys_alloc_page();
    }
    entry->usecount = 0;
    entry->id = ++lastid;
    list_push(shm_table, entry);
    return entry->id;
}
vaddr_t attach_task_to_shm(struct task *task, uint32_t id)
{
    int i, j;
    //Nach id suchen
    for (i = 0; i < list_size(shm_table); i++) {
        if (((shm_table_entry_t *)list_get_element_at(shm_table, i))->id == id) {
            shm_table_entry_t *shmem = (shm_table_entry_t *)list_get_element_at(shm_table, i);
            //Pages suchen
            vaddr_t vaddr = find_contiguous_pages(task->cr3, shmem->pagecount,
                USER_MEM_START, USER_MEM_END);
            //Pages mappen
            for (j = 0; j < shmem->pagecount; j++) {
                map_page(task->cr3, vaddr + j * PAGE_SIZE, shmem->addresses[j],
                    PTE_P | PTE_U | PTE_W);
                
            }
            shmem->usecount++;
            list_push(task->shmids, (void*)id);
            list_push(task->shmaddresses, vaddr);
            return vaddr;
        }
    }
    return 0;
}
void detach_task_from_shm(struct task *task, uint32_t id)
{
    //kprintf("Loese Task von shm %d.\n", id);
    int i, j, k;
    //Einträge suchen
    for (i = 0; i < list_size(task->shmids); i++) {
        if ((uint32_t)list_get_element_at(task->shmids, i) == id) {
            vaddr_t vaddr = list_get_element_at(task->shmaddresses, i);
            for (j = 0; j < list_size(shm_table); j++) {
                if (((shm_table_entry_t *)list_get_element_at(shm_table, j))->id == id) {
                    shm_table_entry_t *shmem = (shm_table_entry_t *)list_get_element_at(shm_table, j);
                    //Pages des Tasks freimachen
                    for (k = 0; k < shmem->pagecount; k++) {
                        unmap_page(task->cr3, vaddr + k * PAGE_SIZE);
                    }
                    list_remove(task->shmids, i);
                    list_remove(task->shmaddresses, i);
                    shmem->usecount--;
                    if (!shmem->usecount) {
                        //kprintf("Shared Memory wird nicht mehr benoetigt.\n");
                        //Shared Memory wird nicht mehr benötigt
                        for (k = 0; k < shmem->pagecount; k++) {
                            phys_mark_page_as_free(shmem->addresses[k]);
                        }
                        free(shmem->addresses);
                        free(shmem);
                        list_remove(shm_table, j);
                    }
                    return;
                }
            }
            return;
        }
    }
}
