/*
 * Copyright (c) 2010 The tyndur Project. All rights reserved.
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

#include <string.h>
#include <syscall.h>

/**
 * Neuen LIO-Service registrieren.
 *
 * @param name   Service-Name
 * @param buffer Zeiger auf den zu benutzenden Ringpuffer fuer die Operationen.
 *               Dieser sollte mit sinnvollen Werten initialisiert sein (z.B
 *               memset(buffer, 0, buffer_size)
 * @param size   Groesse des Puffers
 *
 * @return 0 bei Erfolg, < 0  im Fehlerfall
 */
int lio_srv_service_register(const char* name, void* buffer, size_t size)
{
    int result;
    size_t name_len;

    name_len = strlen(name);
    asm volatile(
        "pushl %5;"
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x10, %%esp;"
        : "=a" (result)
        : "i" (SYSCALL_LIO_SRV_SERVICE_REGISTER), "r" (name), "r" (name_len),
          "r" (buffer), "r" (size));

    return result;
}

/**
 * LIO-Befehlsring für einen gegebenen Tree einrichten
 *
 * @param tree   ID des Trees
 * @param tid    TID des Threads, der den Ring abarbeitet
 * @param buffer Zeiger auf den zu benutzenden Ringpuffer fuer die Operationen.
 *               Dieser sollte mit sinnvollen Werten initialisiert sein (z.B
 *               memset(buffer, 0, buffer_size)
 * @param size   Groesse des Puffers
 *
 * @return 0 bei Erfolg, < 0  im Fehlerfall
 */
int lio_srv_tree_set_ring(lio_usp_tree_t tree, tid_t tid,
                          void* buffer, size_t size)
{
    int result;

    asm volatile(
        "pushl %5;"
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x10, %%esp;"
        : "=a" (result)
        : "i" (SYSCALL_LIO_SRV_TREE_SET_RING), "r" (&tree),
          "r" (tid), "r" (buffer), "r" (size) : "memory");

    return result;
}

/**
 * Ressource fuer Kernel bereitstellen, dabei wird der id-Member eingefuellt.
 *
 * @return 0 bei Erfolg, < 0 im Fehlerfall
 */
int lio_srv_res_upload(struct lio_server_resource* resource)
{
    int result;

    asm volatile(
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
        : "=a" (result)
        : "i" (SYSCALL_LIO_SRV_RES_UPLOAD), "r" (resource));

    return result;
}

/**
 * Neuen Knoten erstellen.
 *
 * @param parent   Elternressource in der der Knoten angelegt werden soll
 * @param resource Ressource auf die der Knoten verweisen soll
 * @param name     Namen des neuen Knoten
 */
int lio_srv_node_add(lio_usp_resource_t parent, lio_usp_resource_t resource,
    const char* name)
{
    int result;
    size_t name_len;

    name_len = strlen(name);
    asm volatile(
        "pushl %5;"
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x10, %%esp;"
        : "=a" (result)
        : "i" (SYSCALL_LIO_SRV_NODE_ADD), "r" (&parent), "r" (&resource),
          "r" (name), "r" (name_len) : "memory");

    return result;
}

/**
 * Knoten loeschen.
 *
 * @param parent Elternressource in der der Kindknoten geloescht werden soll
 * @param name   Name des zu loeschenden Knotens
 *
 * @return 0 bei Erfolg, < 0 im Fehlerfall
 */
int lio_srv_node_remove(lio_usp_resource_t parent, const char* name)
{
    int result;
    size_t name_len;

    name_len = strlen(name);
    asm volatile(
        "pushl %4;"
        "pushl %3;"
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0xc, %%esp;"
        : "=a" (result)
        : "i" (SYSCALL_LIO_SRV_NODE_REMOVE), "r" (&parent), "r" (name),
          "r" (name_len) : "memory");

    return result;

}

/**
 * Kernel ueber abgearbeitete Operation informieren. Das USP-Bit in der
 * Operation muss geloescht sein.
 *
 * @param op Fertig abgearbeitete Operation
 */
void lio_srv_op_done(struct lio_op* op, int status, uint64_t result)
{
    asm volatile(
        "pushl %3;"
        "pushl %2;"
        "pushl %1;"
        "mov %0, %%eax;"
        "int $0x30;"
        "add $0x0c, %%esp;"
        : : "i" (SYSCALL_LIO_SRV_OP_DONE),
            "r" (op), "r" (status), "r" (&result)
        : "memory");
}

/**
 * Auf LIO-Operationen fuer diesen Prozess warten. Solange noch unbearbeitete
 * Operationen existieren, kehrt der Aufruf sofort zurueck.
 */
void lio_srv_wait(void)
{
    asm volatile(
        "mov %0, %%eax;"
        "int $0x30;"
        :: "i" (SYSCALL_LIO_SRV_WAIT));
}
