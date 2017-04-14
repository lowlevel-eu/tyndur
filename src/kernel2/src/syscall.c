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

#include <types.h>
#include <stdint.h>

#include "syscall.h"
#include "syscallno.h"

#include "kernel.h"

syscall_t syscalls[SYSCALL_MAX];

/**
 * Die Syscalls initialisieren.
 */
void syscall_init()
{
    syscall_register(SYSCALL_MEM_ALLOCATE, &syscall_mem_allocate, 3);
    syscall_register(SYSCALL_MEM_ALLOCATE_PHYSICAL,
            &syscall_mem_allocate_physical, 3);
    syscall_register(SYSCALL_MEM_FREE, &syscall_mem_free, 2);
    syscall_register(SYSCALL_MEM_FREE_PHYSICAL, &syscall_mem_free_physical, 2);
    syscall_register(SYSCALL_MEM_INFO, &syscall_mem_info, 0);

    syscall_register(SYSCALL_PM_GET_PID, (void*) &syscall_pm_get_pid, 0);
    syscall_register(SYSCALL_PM_GET_PARENT_PID, (void*)
        &syscall_pm_get_parent_pid, 1);
    syscall_register(SYSCALL_PM_GET_CMDLINE, (void*) syscall_pm_get_cmdline,
        0);

    syscall_register(SYSCALL_PM_SLEEP, &syscall_pm_sleep, 0);
    syscall_register(SYSCALL_PM_WAIT_FOR_RPC, &syscall_pm_wait_for_rpc, 0);

    syscall_register(SYSCALL_PM_P, (void*) &syscall_pm_p, 0);
    syscall_register(SYSCALL_PM_V, (void*) &syscall_pm_v, 1);
    syscall_register(SYSCALL_PM_V_AND_WAIT_FOR_RPC, (void*)
        &syscall_pm_v_and_wait_for_rpc, 0);

    syscall_register(SYSCALL_PM_CREATE_PROCESS, &syscall_pm_create_process, 4);
    syscall_register(SYSCALL_PM_INIT_PAGE, &syscall_init_child_page, 4);
    syscall_register(SYSCALL_PM_INIT_PROC_PARAM_BLOCK, &syscall_init_ppb, 2);
    syscall_register(SYSCALL_PM_EXIT_PROCESS, &syscall_pm_exit_process, 0);

    syscall_register(SYSCALL_PM_ENUMERATE_TASKS,
        &syscall_pm_enumerate_tasks, 0);

#if CONFIG_ARCH == ARCH_I386
    syscall_register(SYSCALL_PM_CREATE_THREAD, &syscall_pm_create_thread, 2);
    syscall_register(SYSCALL_PM_GET_TID, &syscall_pm_get_tid, 0);
    syscall_register(SYSCALL_PM_EXIT_THREAD, &syscall_pm_exit_thread, 0);
#endif

    syscall_register(SYSCALL_MUTEX_LOCK, &syscall_mutex_lock, 1);
    syscall_register(SYSCALL_MUTEX_UNLOCK, &syscall_mutex_unlock, 1);

    syscall_register(SYSCALL_SET_RPC_HANDLER, &syscall_set_rpc_handler, 1);
#if CONFIG_ARCH == ARCH_I386
    syscall_register(SYSCALL_FASTRPC, &syscall_fastrpc, 5);
    syscall_register(SYSCALL_FASTRPC_RET, &syscall_fastrpc_ret, 0);
#endif

    syscall_register(SYSCALL_ADD_TIMER, &syscall_add_timer, 2);


#if CONFIG_ARCH == ARCH_I386
    syscall_register(SYSCALL_PM_REQUEST_PORT, syscall_io_request_port, 2);
    syscall_register(SYSCALL_PM_RELEASE_PORT, syscall_io_release_port, 2);
#endif
    syscall_register(SYSCALL_ADD_INTERRUPT_HANDLER,
        syscall_add_interrupt_handler, 1);

    syscall_register(SYSCALL_PUTSN, (void*) &syscall_putsn, 2);
    syscall_register(SYSCALL_GET_TICK_COUNT, syscall_get_tick_count, 0);

    syscall_register(SYSCALL_SHM_CREATE, &syscall_shm_create, 1);
    syscall_register(SYSCALL_SHM_ATTACH, &syscall_shm_attach, 1);
    syscall_register(SYSCALL_SHM_DETACH, &syscall_shm_detach, 1);
    syscall_register(SYSCALL_SHM_SIZE, &syscall_shm_size, 1);

#if CONFIG_ARCH == ARCH_I386
    syscall_register(SYSCALL_VM86, &syscall_vm86_old, 2);
    syscall_register(SYSCALL_VM86_BIOS_INT, &syscall_vm86, 3);
#endif

    syscall_register(SYSCALL_LIO_RESOURCE, &syscall_lio_resource, 4);
    syscall_register(SYSCALL_LIO_OPEN, &syscall_lio_open, 3);
    syscall_register(SYSCALL_LIO_PIPE, &syscall_lio_pipe, 3);
    syscall_register(SYSCALL_LIO_COMPOSITE_STREAM,
                     &syscall_lio_composite_stream, 3);
    syscall_register(SYSCALL_LIO_CLOSE, &syscall_lio_close, 1);
    syscall_register(SYSCALL_LIO_READ, &syscall_lio_read, 6);
    syscall_register(SYSCALL_LIO_WRITE, &syscall_lio_write, 6);
    syscall_register(SYSCALL_LIO_SEEK, &syscall_lio_seek, 4);
    syscall_register(SYSCALL_LIO_TRUNCATE, &syscall_lio_truncate, 2);
    syscall_register(SYSCALL_LIO_SYNC, &syscall_lio_sync, 1);
    syscall_register(SYSCALL_LIO_READ_DIR, &syscall_lio_read_dir, 5);
    syscall_register(SYSCALL_LIO_MKFILE, &syscall_lio_mkfile, 4);
    syscall_register(SYSCALL_LIO_MKDIR, &syscall_lio_mkdir, 4);
    syscall_register(SYSCALL_LIO_MKSYMLINK, &syscall_lio_mksymlink, 6);
    syscall_register(SYSCALL_LIO_STAT, &syscall_lio_stat, 2);
    syscall_register(SYSCALL_LIO_UNLINK, &syscall_lio_unlink, 3);
    syscall_register(SYSCALL_LIO_SYNC_ALL, &syscall_lio_sync_all, 0);
    syscall_register(SYSCALL_LIO_PASS_FD, &syscall_lio_pass_fd, 2);
    syscall_register(SYSCALL_LIO_PROBE_SERVICE, &syscall_lio_probe_service, 2);
    syscall_register(SYSCALL_LIO_DUP, &syscall_lio_dup, 2);
    syscall_register(SYSCALL_LIO_STREAM_ORIGIN, &syscall_lio_stream_origin, 1);
    syscall_register(SYSCALL_LIO_IOCTL, &syscall_lio_ioctl, 2);

    syscall_register(SYSCALL_LIO_SRV_SERVICE_REGISTER,
        &syscall_lio_srv_service_register, 4);
    syscall_register(SYSCALL_LIO_SRV_TREE_SET_RING,
        &syscall_lio_srv_tree_set_ring, 4);
    syscall_register(SYSCALL_LIO_SRV_RES_UPLOAD,
        &syscall_lio_srv_res_upload, 1);
    syscall_register(SYSCALL_LIO_SRV_NODE_ADD, &syscall_lio_srv_node_add, 4);
    syscall_register(SYSCALL_LIO_SRV_NODE_REMOVE,
        &syscall_lio_srv_node_remove, 3);
    syscall_register(SYSCALL_LIO_SRV_OP_DONE, &syscall_lio_srv_op_done, 3);
    syscall_register(SYSCALL_LIO_SRV_WAIT, &syscall_lio_srv_wait, 0);

    syscalls[KERN_SYSCALL_PM_SLEEP] = (syscall_t) {
        .handler    = &kern_syscall_pm_sleep,
        .arg_count  = 2,
        .privileged = true,
    };
}

/**
 * Einen neuen Syscall registrieren
 *
 * @param number Syscallnummer (syscallno.h!)
 * @param handler Adresse der Handler-Funktion
 * @param arg_count Anzahl der Argumente, die fuer diesen Syscall uebergeben
 *                  werden muessen.
 */
void syscall_register(syscall_arg_t number, void* handler, size_t arg_count)
{
    // Ein Syscall mit einer zu grossen ID wuerde dazu fuehren, dass Teile von
    // Kernel-Daten ueberschrieben wuerden
    if (number >= SYSCALL_MAX) {
        panic("Es wurde versucht einen Syscall mit einer Nummer groesser als"
                "SYSCALL_MAX zu reservieren!");
    }
    syscalls[number].handler = handler;
    syscalls[number].arg_count = arg_count;
    syscalls[number].privileged = false;
}
