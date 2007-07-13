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
#include "pci.h"
#include "ports.h"

#include "network.h"
#include "rtl8139.h"
#include "ethernet.h"
#include "arp.h"
#include "ip.h"

#define _USE_START_
#include "init.h"

void register_network_driver();

int main(int argc, char* argv[])
{
    qword timeout = get_tick_count() + 5000000;
    while (init_service_get("pci") == 0)
    {
        yield();
        if(get_tick_count() > timeout)
        {
            puts("[ RTL8139 ] Konnte pci nicht finden!");
            return -1;
        }
    }

    init_rtl8139();
    arp_init();
    ip_init();

    register_receive_handler
        (the_one_and_only_netcard, ETH_TYPE_ARP, arp_receive);
    
    register_receive_handler
        (the_one_and_only_netcard, ETH_TYPE_IP, ip_receive);

/*
    printf("============ TEST Beginn ===============\n");

    printf("MAC von 10.0.2.2: %llx\n", 
        arp_resolve(the_one_and_only_netcard, 0x0202000a));
    printf("MAC von 10.0.2.3: %llx\n", 
        arp_resolve(the_one_and_only_netcard, 0x0302000a));

    printf("============ TEST Ende ===============\n");
*/
    
    timeout = get_tick_count() + 5000000;
    while (init_service_get("tcpip") == 0)
    {
        yield();
        if(get_tick_count() > timeout)
        {
            puts("[ RTL8139 ] Konnte tcpip nicht finden!");
            return -1;
        }
    }

    register_network_driver();

    while(1) {
        wait_for_rpc();
    }
}

void register_network_driver()
{
    struct net_driver driver = {
        .device = 0,
        .ip = 0x0b01a8c0
    };

    char buffer[8 + sizeof(driver)];

    memcpy(buffer, RPC_IP_REGISTER_DRIVER, 8);
    memcpy(buffer + 8, &driver, sizeof(driver));

    send_message(
        init_service_get("tcpip"),
        512,
        0,
        8 + sizeof(driver),
        buffer
    );
}
