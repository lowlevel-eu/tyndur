/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
#include "string.h"
#include "init.h"
#include "stdlib.h"

/**
 * Dem aktuellen Prozess beim Init-Prozess als Treiber anmelden.
 * @param name Der Treibername
 */
void init_service_register(char* name)
{
    const char* rpc_name = "SERV_REG";
    char* msg = malloc(strlen(rpc_name) + strlen(name) + 1);
    
    memcpy(msg, rpc_name, strlen(rpc_name));
    memcpy((char*)((dword)msg + strlen(rpc_name)), name, strlen(name) + 1);
    
    
    send_message(INIT_PID, 512, 0, strlen(msg) + 1, msg);
}


/**
 * Die PID des Treibers beim Init-Prozess erfragen.
 * @param name Der Treibermame
 * @return Die Prozess ID des Treibers
 */
pid_t init_service_get(char* name)
{
    return rpc_get_dword(INIT_PID, "SERV_GET", strlen(name) + 1, name);
}


/**
 * 
 * @param result Rueckgabewert des Tasks
 */
void init_process_exit(int result)
{
    const char* rpc_name = "SERV_EXI";
    char* msg = malloc(strlen(rpc_name) + 4 + 1);
    
    memcpy(msg, rpc_name, strlen(rpc_name));
    memcpy((char*)((dword)msg + strlen(rpc_name)), &result, 4);
    msg[strlen(rpc_name) + 4] = 0;
    
    
    send_message(INIT_PID, 512, 0, strlen(msg), msg);

}

