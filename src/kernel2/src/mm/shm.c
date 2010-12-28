/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
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
#include <stdlib.h>
#include <stddef.h>
#include <collections.h>

#include "mm.h"
#include "cpu.h"
#include "tasks.h"

struct shm_desc {
    uint64_t            id;
    struct tree_item    tree_item;

    size_t              num_pages;
    list_t*             processes;
};

struct shm_process {
    struct shm_desc*    shm;
    pm_process_t*       process;
    vaddr_t             vaddr;
};

static tree_t* shms = NULL;
static uint32_t shm_last_id = 0;

static void shm_destroy_task(pm_process_t* process, void* prv);

/**
 * Initialisiert die SHM-Verwaltung
 */
void shm_init(void)
{
    shms = tree_create(struct shm_desc, tree_item, id);
}

/**
 * Neuen Shared Memory reservieren
 *
 * @return ID des neu angelegten Shared Memory. Der Aufrufer muss anschliessend
 * syscall_shm_attach aufrufen, um auf den Speicher zugreifen zu koennen.
 */
uint32_t shm_create(size_t size)
{
    uint32_t id = ++shm_last_id;
    struct shm_desc* shm;

    while (tree_search(shms, id)) {
        id++;
    }

    shm = calloc(1, sizeof(*shm));
    shm->id = id;
    shm->num_pages = NUM_PAGES(size);
    shm->processes = list_create();

    tree_insert(shms, shm);

    return id;
}

/**
 * Bestehenden Shared Memory oeffnen.
 *
 * @param process Prozess, der den Shared Memory oeffnet
 * @param id ID des zu oeffnenden Shared Memory
 * @return Virtuelle Adresse des geoeffneten Shared Memory; NULL, wenn der
 * Shared Memory mit der angegebenen ID nicht existiert.
 */
void* shm_attach(pm_process_t* process, uint32_t id)
{
    struct shm_desc* shm = tree_search(shms, id);
    void* ret;
    struct shm_process* shm_proc;

    if (shm == NULL) {
        return NULL;
    }

    if (list_is_empty(shm->processes)) {
        // Erster Prozess, der den SHM oeffnet: Speicher allozieren
        ret = mmc_valloc(&process->context, shm->num_pages,
            MM_FLAGS_USER_DATA);
    } else {
        // Ansonsten Mapping von einem anderen Prozess uebernehmen
        struct shm_process* first = list_get_element_at(shm->processes, 0);
        ret = mmc_automap_user(&process->context,
            &first->process->context, first->vaddr, shm->num_pages,
            MM_USER_START, MM_USER_END, MM_FLAGS_USER_DATA);
    }

    if (ret == NULL) {
        return NULL;
    }

    shm_proc = calloc(1, sizeof(*shm_proc));
    shm_proc->shm = shm;
    shm_proc->process = process;
    shm_proc->vaddr = ret;
    list_push(shm->processes, shm_proc);

    if (process->shm == NULL) {
        process->shm = list_create();
        pm_register_on_destroy(process, shm_destroy_task, NULL);
    }
    list_push(process->shm, shm_proc);

    return ret;
}

/**
 * Shared Memory schliessen. Wenn keine Prozesse den Shared Memory mehr
 * geoeffnet haben, wird er geloescht.
 *
 * @param process Prozess, der den Shared Memory schliesst
 * @param id ID des zu schliessenden Shared Memory
 */
void shm_detach(pm_process_t* process, uint32_t id)
{
    struct shm_desc* shm = tree_search(shms, id);
    struct shm_process* shm_proc;
    int i;

    if (shm == NULL) {
        return;
    }

    // Aus der SHM-Liste des Prozesses austragen
    for (i = 0; (shm_proc = list_get_element_at(process->shm, i)); i++) {
        if (shm_proc->shm == shm) {
            list_remove(process->shm, i);
            break;
        }
    }

    // Aus der Prozessliste des SHM austragen
    for (i = 0; (shm_proc = list_get_element_at(shm->processes, i)); i++) {
        if (shm_proc->process == process) {
            list_remove(shm->processes, i);
            goto found;
        }
    }

    return;

found:

    // Wenn es der letzte Benutzer war, SHM loeschen
    if (list_is_empty(shm->processes)) {
        for (i = 0; i < shm->num_pages; i++) {
            pmm_free(mmc_resolve(&process->context,
                shm_proc->vaddr + (i * PAGE_SIZE)), 1);
        }

        list_destroy(shm->processes);
        shm->processes = NULL;
        tree_remove(shms, shm);
    }

    // Virtuellen Speicher freigeben
    mmc_unmap(&process->context, shm_proc->vaddr, shm->num_pages);


    free(shm_proc);
    if (shm->processes == NULL) {
        free(shm);
    }

    return;
}

/**
 * Gibt die Groesse des SHM-Bereichs in Bytes zurueck
 */
size_t shm_size(uint32_t id)
{
    struct shm_desc* shm = tree_search(shms, id);

    if (shm == NULL) {
        return 0;
    }

    return shm->num_pages * PAGE_SIZE;
}

/**
 * Schliesst alle SHM-Bereiche eines Prozesses
 */
static void shm_destroy_task(pm_process_t* process, void* prv)
{
    struct shm_process* shm_proc;

    if (process->shm == NULL) {
        return;
    }

    while ((shm_proc = list_get_element_at(process->shm, 0))) {
        shm_detach(process, shm_proc->shm->id);
    }

    list_destroy(process->shm);
}
