/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
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
#include "rpc.h"
#include "syscall.h"
#include "string.h"

#define MAX_MESSAGE_HANDLERS 10

// Diese Deklaration ist so eigentlich nicht richtig, der RPC-Handler nimmt
// durchaus Parameter. Allerdings folgt das ganze sowieso nicht dem
// C-Aufrufschema, so daﬂ jede andere Deklaration genauso falsch w‰re.
extern void rpc_handler();
void putn(unsigned long long x, int radix, int pad, char padchar);

typedef struct {
    char name[8];
    void (*handler)(int, void*);
} msg_handler_t;

void (*intr_handler[256])() = { NULL };
msg_handler_t message_handler[MAX_MESSAGE_HANDLERS];

void init_messaging()
{
    dword i;

    set_rpc_handler(&rpc_handler);
    for (i = 0; i < MAX_MESSAGE_HANDLERS; i++) {
        message_handler[i].name[0] = '\0';
        message_handler[i].handler = NULL;
    }
}

bool register_message_handler(char* fnname, void (*handler)(int, void*))
{
    char padded_fnname[8];
    dword i, index;

    // Es sind genau acht Byte f¸r den Funktionsnamen reserviert
    strncpy(padded_fnname, fnname, 8);

    // Freien Index im message_handler-Array besorgen und pr¸fen,
    // ob nicht bereits eine Funktion mit demselben Namen
    // existiert.
    index = MAX_MESSAGE_HANDLERS;
    for (i= 0; i < MAX_MESSAGE_HANDLERS; i++) {
        if (message_handler[i].name == NULL) {
            index = i;
        } else if (strncmp(padded_fnname, message_handler[i].name, 8) == 0) {
            return FALSE;
        }
    }
    
    if (index == MAX_MESSAGE_HANDLERS) {
        return FALSE;
    }

    strncpy(message_handler[index].name, padded_fnname, 8);
    message_handler[index].handler = handler;

    return TRUE;
}

void register_intr_handler(byte intr, void (*handler)())
{
    intr_handler[intr] = handler;
    add_intr_handler(intr);
}

void c_rpc_handler(dword data_size, char* data)
{
    dword function = *((dword*) data);

    if (function < 256) {
        intr_handler[function]();
    } else if (function == 512) {
        // RPC by function name
        // TODO
    }
}
