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

#include <init.h>
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

struct module_options {
    dword   ip;
};

void process_parameter(struct module_options* options, char* param);

int init(int argc, char* argv[])
{
    struct module_options options = {
        // TODO Auf 0 setzen und am Ende prüfen und ggf. einfach was 
        // freies suchen
        .ip = 0x0b01a8c0
    };

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

    int i;
    for (i = 1; i < argc; i++) {
        process_parameter(&options, argv[i]);
    }

    init_rtl8139(options.ip);
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

    register_netcard(0, options.ip);

    return 0;
}

int main(int argc, char* argv[])    
{
    if (init(argc, argv)) {
        return 1;
    }

    while(1) {
        wait_for_rpc();
    }
}

void process_parameter(struct module_options* options, char* param)
{
    printf("rtl8139-Parameter: %s\n", param);

    if (strncmp(param, "ip=", 3) == 0) {
        dword ip = string_to_ip(&param[3]);        
        printf("IP-Adresse: %08x\n", ip);
        options->ip = ip;
    } else {
        printf("Unbekannter Parameter %s\n", param);
    }
}        
