/*
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Burkhard Weseloh.
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
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
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

#include <elf32.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <lost/config.h>
#include "kprintf.h"
#include "multiboot.h"
#include "debug.h"

static uint32_t debug_flags = 0;

/**
 * Ueberprueft ob ein bestimmtes Debug-Flag gesetzt ist
 *
 * @param flag Flag-Nummer
 *
 * @return TRUE wenn gesetzt, FALSE sonst
 */
bool debug_test_flag(uint32_t flag)
{
    return ((debug_flags & flag) != 0);
}

/**
 * Gibt die Debug-Meldung aus, wenn das Flag gesetzt ist.
 *
 * @param flag Flag-Nummer
 */
void debug_print(uint32_t flag, const char* message)
{
    if (debug_test_flag(flag)) {
        kprintf("DEBUG: %s\n", message);
    }
}

///Setzt die richtigen Debug-Flags anhand der Commandline vom Bootloader
void debug_parse_cmdline(char* cmdline)
{
    char* pos = strstr(cmdline, "debug=");
    debug_flags = 0;

    if(pos == NULL) return;

    //Debug= ueberspringen
    pos += 6;
    while((*pos != 0) && (*pos != ' '))
    {
        switch(*pos)
        {
            case 'i':
                debug_flags |= DEBUG_FLAG_INIT;
                break;

            case 'p':
                debug_flags |= DEBUG_FLAG_PEDANTIC;
                break;

            case 's':
                debug_flags |= DEBUG_FLAG_STACK_BACKTRACE;
                break;

            case 'c':
                debug_flags |= DEBUG_FLAG_SYSCALL;
                break;
        }
        pos++;
    }
}

