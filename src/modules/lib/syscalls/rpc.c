/*
 * Copyright (c) 2006-2009 The tyndur Project. All rights reserved.
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
#include <syscall.h>
#include <errno.h>

void rpc(pid_t pid)
{
    uint32_t result = 0;

    do {
        asm(
            "pushl $0;"
            "pushl %1;"
            "mov $51, %%eax;"
            "int $0x30;"
            "add $0x8, %%esp;"
        : "=a" (result) : "g" (pid), "0" (result));
    } while (result != 0);
}

int send_message(pid_t pid, uint32_t function, uint32_t correlation_id,
    uint32_t len, char* data)
{
    int32_t result = 0;

    // Auf die Variable wird zwar sonst nirgends schreibend zugegriffen, aber
    // das volatile ist noetig, weil gcc sonst der Meinung ist, der Arrayinhalt
    // wuerde nicht benutzt (dem asm-Block wird nur der Pointer uebergeben).
    // Ohne volatile laesst gcc mit -O1 die Initialisierung des Arrays daher
    // grosszuegig unter den Tisch fallen.
    volatile uint32_t metadata[2] = { function, correlation_id };

    do {
        asm(
            "pushl %6;"
            "pushl %5;"
            "pushl %4;"
            "pushl %3;"
            "pushl %2;"
            "mov %1, %%eax;"
            "int $0x30;"
            "add $0x14, %%esp;"
        : "=a" (result)
        : "i" (SYSCALL_FASTRPC), "g" (pid), "i" (0x8), "g" (metadata),
            "g" (len), "g" (data), "0" (result));

    } while (result == -EAGAIN);

    return result;
}

