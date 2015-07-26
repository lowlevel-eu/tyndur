/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
#include <stdbool.h>
#include <rpc.h>
#include <assert.h>
#include <collections.h>
#include <syscall.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

struct wait_child {
    pid_t pid;
    bool running;
    int status;
};

// Liste mit den Kindprozessen
static list_t* wait_list = NULL;

static void rpc_child_exit(pid_t pid, uint32_t correlation_id, size_t data_size,
    void* data);

/**
 * Wait-Funktionen initialisieren
 */
void init_waitpid(void)
{
    wait_list = list_create();
    register_message_handler("CHL_EXIT", &rpc_child_exit);
}

/**
 * Kindprozess aus der liste hoeln
 */
static struct wait_child* wait_child_get(pid_t pid)
{
    int i;
    struct wait_child* wait_child;
    for (i = 0; (wait_child = list_get_element_at(wait_list, i)); i++) {
        if (wait_child->pid == pid) {
            return wait_child;
        }
    }

    return NULL;
}

/**
 * Neuen Kindprozess registrieren
 *
 * Nicht static weil auch in init_execute benutzt
 */
void wait_child_add(pid_t pid)
{
    p();
    struct wait_child* wait_child = wait_child_get(pid);

    // Es kann sein, dass der Prozess schon terminiert hat, wenn diese Funktion
    // aufgerufen wird!
    if (wait_child != NULL) {
        goto out;
    }

    wait_child = malloc(sizeof(struct wait_child));
    assert(wait_child != NULL);

    wait_child->pid = pid;
    wait_child->running = true;
    wait_child->status = -1;
    list_push(wait_list, wait_child);

out:
    v();
}

/**
 * Handle fuer Waitpid aus der Liste entfernen
 */
static void wait_child_del(pid_t pid)
{
    int i;
    struct wait_child* wait_child;
    for (i = 0; (wait_child = list_get_element_at(wait_list, i)); i++) {
        if (wait_child->pid == pid) {
            list_remove(wait_list, i);
            return;
        }
    }
}

/**
 * RPC der von den Kindprozessen gesendet wird, um uns den Exit-Code
 * mitzuteilen
 */
static void rpc_child_exit(pid_t pid, uint32_t correlation_id, size_t data_size,
    void* data)
{
    struct wait_child* wait_child;

    // Pruefen ob der RPC wirklich die richtige Groesse hat
    if (data_size != sizeof(int)) {
        return;
    }
    
    p();
    // Wenn der Prozess noch nicht als Kindprozess registriert wurde, wird das
    // jetzt gemacht
    wait_child = wait_child_get(pid);
    if (wait_child == NULL) {
        wait_child_add(pid);
    }
    wait_child = wait_child_get(pid);
    
    assert(wait_child != NULL);
    wait_child->status = *((int *) data);
    wait_child->running = false;
    v();
}

/**
 * Wartet bis der angegebene Prozess terminiert
 */
pid_t waitpid(pid_t pid, int* status, int options)
{
    struct wait_child* wait_child;
    // Status eines Bestimmten Prozesses gewuenscht
    if (pid > 0) {
        wait_child = wait_child_get(pid);

        // Nicht unser Kindprozess
        if (wait_child == NULL) {
            errno = ECHILD;
            return -1;
        }
        
        // Warten bis der Prozess terminiert
        if ((options & WNOHANG) == 0) {
            while (wait_child->running && (get_parent_pid(pid) != 0)) {
                wait_for_rpc();
            }
        } else if (wait_child->running) {
            return 0;
        }
        
        if (status != NULL) {
            *status = wait_child->status;
        }
        wait_child_del(pid);
        return pid;
    } else {
        // Irgend ein Prozess (FIXME)
        printf("waitpid(-1)\n");
        errno = ECHILD;
        return -1;
    }
}


/**
 * Wartet bis ein Kindprozess terminiert
 *
 * @param status Pointer auf Variable fuer Rueckgabewert
 *
 * @return PID des Kindprozesses
 */
pid_t wait(int* status)
{
    return waitpid(-1, status, 0);
}

