/*
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
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

#ifndef RPC_H
#define RPC_H

#include "types.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_MESSAGE_HANDLERS 32
#define RPC_FUNCTION_NAME_LENGTH 8

#define RPC_SIGNALS_START 256
#define RPC_SIGNALS_END 511
#define RPC_MESSAGE 512
#define RPC_RESPONSE 513
#define RPC_TIMER 514

#define RPC_TRPC_FIRST 1024
#define RPC_TRPC_LAST 4095

typedef void (*handler_function_t)(pid_t, uint32_t, size_t, void*);


typedef struct {
    uint32_t pid;
    uint32_t correlation_id;
    size_t data_length;
    void* data;
} response_t;

extern handler_function_t rpc_response_handler;

void init_messaging(void);
bool register_message_handler(char* fnname, handler_function_t handler);
void register_intr_handler(uint8_t intr, void (*handler)(uint8_t));

uint32_t rpc_get_dword(pid_t pid, char* function_name, size_t data_length,
    char* data);
int rpc_get_int(pid_t pid, char* function_name, size_t data_length, char* data);
char* rpc_get_string(pid_t pid, char* function_name, size_t data_length,
    char* data);
response_t* rpc_get_response(pid_t pid, char* function_name, size_t data_length,
    char* data);

void rpc_send_response(pid_t pid, uint32_t correlation_id, size_t len,
    char* data);
void rpc_send_dword_response(pid_t pid, uint32_t correlation_id,
    uint32_t response);
void rpc_send_int_response(pid_t pid, uint32_t correlation_id, int response);
void rpc_send_string_response(pid_t pid, uint32_t correlation_id,
    char* response);

uint32_t timer_register(void (*callback)(void), uint32_t usec);
void timer_cancel(uint32_t timer_id);

#endif
