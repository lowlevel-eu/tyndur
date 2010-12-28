/*
 * Copyright (c) 2010 The tyndur Project. All rights reserved.
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

#include <stdbool.h>
#include <syscall.h>
#include <loader.h>
#include <lock.h>

static struct {
    pid_t   pid;
    char*   args;
    pid_t   parent_pid;
    lock_t  lock;
} loader_state;

/**
 * Speicher allozieren um ihn spaeter in einen neuen Prozess zu mappen. Diese
 * Funktion sollte nicht fuer "normale" Allokationen benutzt werden, da immer
 * ganze Pages alloziert werden.
 *
 * @param size minimale Groesse des Bereichs
 *
 * @return Adresse, oder NULL falls ein Fehler aufgetreten ist
 */
vaddr_t loader_allocate_mem(size_t size)
{
    return mem_allocate(size, 0);
}

/**
 * Ein Stueck Speicher in den Prozess mappen. Dieser darf dazu noch nicht
 * gestartet sein. Der Speicher muss zuerst mit loader_allocate_mem alloziert
 * worden sein, denn sonst kann nicht garantiert werden, dass der Speicher
 * uebertragen werden kann.
 *
 * @param process PID des Prozesses
 * @param dest_address Adresse an die der Speicher im Zielprozess soll
 * @param src_address Adresse im aktuellen Kontext die uebetragen werden soll
 * @param size Groesse des Speicherbereichs in Bytes
 *
 * @return true, wenn der bereich gemappt wurde, FALSE sonst
 */
bool loader_assign_mem(pid_t process, vaddr_t dest_address,
    vaddr_t src_address, size_t size)
{
    init_child_page(loader_state.pid, dest_address, src_address, size);
    return true;
}


/**
 * Erstellt einen neuen Thread.
 *
 * @param process PID
 * @param address Einsprungsadresse des Threads
 *
 * @return bool true, wenn der Thread erstellt wurde, FALSE sonst
 */
bool loader_create_thread(pid_t process, vaddr_t address)
{
    loader_state.pid = create_process((uintptr_t) address, 0,
        loader_state.args, loader_state.parent_pid);
    return loader_state.pid != 0;
}

/**
 * Erstellt einen neuen Prozess aus einer ELF-Datei
 */
pid_t load_single_module(void* image, size_t image_size, char* args,
    pid_t parent_pid, int ppb_shm_id)
{
    p();
    lock(&loader_state.lock);

    loader_state.args = args;
    loader_state.parent_pid = parent_pid;
    if (!loader_elf32_load_image(0, image, image_size)) {
        loader_state.pid = 0;
        goto fail;
    }

    if (ppb_shm_id) {
        init_child_ppb(loader_state.pid, ppb_shm_id);
    }

    unblock_process(loader_state.pid);

fail:
    unlock(&loader_state.lock);
    v();
    return loader_state.pid;
}
