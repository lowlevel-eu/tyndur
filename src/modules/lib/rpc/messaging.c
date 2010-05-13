/*  
 * Copyright (c) 2006, 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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
#include "types.h"
#include "rpc.h"
#include "syscall.h"
#include "string.h"
#include <signal.h>

// Diese Deklaration ist so eigentlich nicht richtig, der RPC-Handler nimmt
// durchaus Parameter. Allerdings folgt das ganze sowieso nicht dem
// C-Aufrufschema, so daﬂ jede andere Deklaration genauso falsch w‰re.
extern void librpc_rpc_handler(void);

void timer_callback(uint32_t timer_id);

typedef struct {
    char name[RPC_FUNCTION_NAME_LENGTH];
    handler_function_t handler;
} msg_handler_t;

void (*intr_handler[256])(uint8_t) = { NULL };
msg_handler_t message_handler[MAX_MESSAGE_HANDLERS];

handler_function_t rpc_response_handler = NULL;

static pid_t my_pid;

void init_messaging()
{
    uint32_t i;

    my_pid = get_pid();

    set_rpc_handler(&librpc_rpc_handler);
    for (i = 0; i < MAX_MESSAGE_HANDLERS; i++) {
        message_handler[i].name[0] = '\0';
        message_handler[i].handler = NULL;
    }
}

bool register_message_handler(char* fnname, handler_function_t handler)
{
    char padded_fnname[RPC_FUNCTION_NAME_LENGTH];
    uint32_t i, index;

    // Es sind genau acht Byte f¸r den Funktionsnamen reserviert
    memset(padded_fnname, 0, RPC_FUNCTION_NAME_LENGTH);
    strncpy(padded_fnname, fnname, RPC_FUNCTION_NAME_LENGTH);

    // Freien Index im message_handler-Array besorgen und pr¸fen,
    // ob nicht bereits eine Funktion mit demselben Namen
    // existiert.
    index = MAX_MESSAGE_HANDLERS;
    for (i = 0; i < MAX_MESSAGE_HANDLERS; i++) {
        if (message_handler[i].name[0] == '\0') {
            index = i;
        } else if (strncmp(padded_fnname, message_handler[i].name, RPC_FUNCTION_NAME_LENGTH) == 0) {
            return false;
        }
    }
    
    if (index == MAX_MESSAGE_HANDLERS) {
        return false;
    }

    strncpy(message_handler[index].name, padded_fnname, RPC_FUNCTION_NAME_LENGTH);
    message_handler[index].handler = handler;

    return true;
}

void register_intr_handler(uint8_t intr, void (*handler)(uint8_t))
{
    intr_handler[intr] = handler;
    add_intr_handler(intr);
}

void librpc_c_rpc_handler(uint32_t caller_pid, uint32_t data_size, char* data)
{
    int i;
    
    // Die ersten 8 Bytes der Nachricht enthalten die Funktionsnummer
    // und die Zuordnungs-ID. Die eigentlichen Daten beginnen bei
    // (data + 2*sizeof(uint32_t)).
    uint32_t function = *((uint32_t*) data);
    uint32_t correlation_id = *((uint32_t*) (data  + 4));

    //printf("rpc from %d: function:%d,  correlation_id:%d,  size:%d,  data:0x%x\n", caller_pid, function, correlation_id, data_size - (2*sizeof(dword) + RPC_FUNCTION_NAME_LENGTH), (dword) data + (2*sizeof(dword) + RPC_FUNCTION_NAME_LENGTH));

    if (function < 256) {
        // Interrupt-Handler
        if ((intr_handler[function] != NULL) && (caller_pid == my_pid)) {
            intr_handler[function](function);
        }
    } else if ((function >= RPC_SIGNALS_START) && (function <=
        RPC_SIGNALS_END))
    {
        raise(function - RPC_SIGNALS_START);
    } else if (function == RPC_MESSAGE) {
        // RPC by function name
        //
        // In diesem Fall kommen vor den Nutzdaten noch weitere
        // RPC_FUNCTION_NAME_LENGTH Bytes Funktionsname, die beim Aufruf
        // der Handlerfunktion ¸bersprungen werden m¸ssen
        
        //if (my_pid == 7) printf("%8s\n\n", data + 8);
        
        if (strnlen(data + 8, RPC_FUNCTION_NAME_LENGTH) == 0) {
            return;
        }

        for (i = 0; i < MAX_MESSAGE_HANDLERS; i++) {
            if (strncmp(data + 8, message_handler[i].name, RPC_FUNCTION_NAME_LENGTH) == 0) {
                message_handler[i].handler(
                    caller_pid,
                    correlation_id,
                    data_size - (2*sizeof(uint32_t) + RPC_FUNCTION_NAME_LENGTH),
                    data      + (2*sizeof(uint32_t) + RPC_FUNCTION_NAME_LENGTH)
                );
                break;
            }
        }
    } else if (function == RPC_RESPONSE) {
        // Antwort auf eine synchrone RPC-Anfrage
        rpc_response_handler(
            caller_pid,
            correlation_id,
            data_size   - 2*sizeof(uint32_t),
            data        + 2*sizeof(uint32_t)
        );
    /*
    } else if ((function >= RPC_TRPC_FIRST) && (function <= RPC_TRPC_LAST)) {
        trpc_server_call(
            caller_pid,
            correlation_id,
            data_size   - 2*sizeof(uint32_t),
            data        + 2*sizeof(uint32_t)
        );
    */        
    } else if (function == RPC_TIMER) {
        // Ein Timer ist abgelaufen
        timer_callback(correlation_id);                
    }
}
