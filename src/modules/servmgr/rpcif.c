/*
 * Copyright (c) 2008-2009 The tyndur Project. All rights reserved.
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

#include <stdint.h>
#include <types.h>
#include <syscall.h>
#include <rpc.h>
#include <string.h>
#include <stdio.h>
#include <init.h>
#include "servmgr.h"

static void rpc_needserv(pid_t pid, uint32_t cid, size_t data_size, void* data);
static void rpc_vterm(pid_t pid, uint32_t cid, size_t data_size, void* data);


void rpcif_init()
{
    register_message_handler("NEEDSERV", rpc_needserv);
    register_message_handler("VTERM   ", rpc_vterm);
    init_service_register("servmgr");
}

/**
 * RPC um einen Service zu startem. Falls servmgr noch nicht konfiguriert ist,
 * wird einfach gewartet, bis der Dienst laeuft.
 */
static void rpc_needserv(pid_t pid, uint32_t cid, size_t data_size, void* data)
{
    char* name = data;
    bool result;

    // Pruefen ob der Name nullterminiert ist
    if (!data_size || (name[data_size - 1] != 0)) {
        rpc_send_dword_response(pid, cid, 0);
        return;
    }

    if (config_done) {
        result = service_start(name);
    } else {
        // Noch nicht konfiguriert. Jetzt bleibt nur hoffen, dass der Dienst
        // schon geladen wurde, sich aber noch nicht registriert hat.
        result = service_wait(name, 10000);
    }

    rpc_send_dword_response(pid, cid, result);
}

/**
 * RPC um mitzuteilen, dass vterm jetzt da ist
 */
static void rpc_vterm(pid_t pid, uint32_t cid, size_t data_size, void* data)
{
    service_io_stop_kputsn();
    rpc_send_dword_response(pid, cid, 0);
    service_io_flush();
}
