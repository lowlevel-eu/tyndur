/*  
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
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
#ifndef _INIT_H_
#define _INIT_H_

#include <types.h>
#include <stdio.h>

#define INIT_PID 1

extern void stdio_init(void);


void init_service_register(char* name);
pid_t init_service_get(char* name);
char *init_service_get_name(pid_t pid);

void init_process_exit(int result);
pid_t init_execute(const char* cmd);




#ifdef _USE_START_
#undef _USE_START_
#warning _USE_START_ sollte nicht mehr verwendet werden
#else
    struct service_s
    {
        pid_t   pid;
        char*   name;
    };
    
    struct service_s* get_service_by_name(const char* name);
    struct service_s* get_service_by_pid(pid_t pid);

#endif

struct init_dev_desc {
    char    name[64];
    int     type;
    size_t  bus_data_size;
    uint8_t bus_data[];
};

int init_dev_register(struct init_dev_desc* dev, size_t size);
int init_dev_list(struct init_dev_desc** devs);

#endif

