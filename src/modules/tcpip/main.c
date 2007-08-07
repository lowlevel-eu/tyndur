/*  
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
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

#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"

#include "network.h"
#include "ip.h"
#include "tcp.h"
#include "lostio_if.h"


#define _USE_START_
#include "init.h"

struct module_options {
    dword   gateway;
};

static struct module_options options = {
    .gateway = 0x0202000a
};

void init_routing(pid_t driver, dword device, dword ip);
void init_ip_receive(pid_t driver);
void rpc_register_driver(pid_t pid, dword cid, size_t data_size, void* data);
void process_parameter(struct module_options* options, char* param);


int main(int argc, char* argv[])
{
    int i;
    for (i = 1; i < argc; i++) {
        process_parameter(&options, argv[i]);
    }

    init_ip();
    init_tcp();
    init_lostio_interface();

    register_message_handler(RPC_IP_REGISTER_DRIVER, rpc_register_driver);
    
    init_service_register("tcpip");

    while(1) {
        wait_for_rpc();
    }
}

void rpc_register_driver(pid_t pid, dword cid, size_t data_size, void* data)
{
    if (data_size != sizeof(struct net_driver)) {
        return;
    }

    struct net_driver* driver = data;

    init_routing(pid, driver->device, driver->ip);
    init_ip_receive(pid);
}

void init_routing(pid_t driver, dword device, dword ip)
{
    struct routing_entry entry = {
        .target = 0,
        .subnet = 0,
        .gateway = options.gateway,
        .driver = driver,
        .device = device,
        .localip = ip
    };

    struct routing_entry* pentry = malloc(sizeof(entry));
    *pentry = entry;

    add_routing_entry(pentry);
}

void init_ip_receive(pid_t driver)
{
    send_message(
        driver,
        512,
        0,
        8,
        RPC_IP_REGISTER_RECEIVER
    );
}

#if 0
void test_ping()
{
    dword ping_size = 3 * sizeof(dword);
    dword ping[3] = {
        0xFFF70008,
        0x00000000,
        0x00000000
    };

    ip_send(0x0202000a, IP_PROTO_ICMP, ping, ping_size);
}
#endif

void process_parameter(struct module_options* options, char* param)
{
    if (strncmp(param, "gw=", 3) == 0) {
        dword gateway = string_to_ip(&param[3]);        
        printf("Gateway: %08x\n", gateway);
        options->gateway = gateway;
    } else {
        printf("Unbekannter Parameter %s\n", param);
    }
}        
