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
#include "tasks.h"
#include "tss.h"
#include "stdlib.h"

/* Gesetztes Bit: Port wird benutzt */
byte global_port_bitmap[IO_BITMAP_LENGTH / 8] = { 0 };

void init_io_ports()
{
    // Setze IO-Privilege-Level auf 0
    asm volatile(
        "pushf;"
        "pop %eax;"
        "andl $0xFFFFCFFF, %eax;"
        "push %eax;"
        "popf;"
    );
}
           
static bool io_port_request(struct task* task, dword port)
{
    dword index = port / 8;
    byte bit = 1 << (port % 8);

    if (port >= IO_BITMAP_LENGTH) {
        return FALSE;
    }

    if (global_port_bitmap[index] & bit) {
        return FALSE;
    }

    byte* io_bitmap = task->io_bitmap;
    if (io_bitmap == NULL) {
        io_bitmap = task->io_bitmap = malloc(IO_BITMAP_LENGTH / 8);
    }

    global_port_bitmap[index] |= bit;
    io_bitmap[index] &= ~bit;

    return TRUE;
}

bool io_ports_request(struct task* task, dword port, dword length)
{
    while (length--) {
        if (!io_port_request(task, port++)) {
            // TODO Bereits reservierte Ports wieder freigeben
            return FALSE;
        }
    }

    return TRUE;
}

bool io_ports_release(struct task* task, dword port, dword length)
{
    // TODO Implementieren
    return TRUE;
}

bool io_ports_release_all(struct task* task, dword port, dword length)
{
    // TODO Implementieren
    return TRUE;
}
