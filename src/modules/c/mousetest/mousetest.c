/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Mathias Gottschlag.
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

#include <init.h>

#include <stdbool.h>
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "rpc.h"
#include "io.h"
#include "dir.h"
#include "lostio.h"
#include <lost/config.h>

int main(int argc, char* argv[])
{
    uint64_t timeout = get_tick_count() + 20000000;
    while ((init_service_get("kbc") == 0) || (init_service_get("console") == 0))
    {
        yield();
        if(get_tick_count() > timeout)
        {
            if (init_service_get("kbc") == 0)
            {
                puts("[ MOUSETEST ] Konnte den Maustreiber nicht finden!");
            }
            
            if(init_service_get("console") == 0)
            {
              puts("[ MOUSETEST ] Konnte den Konsolentreiber nicht finden!");
            }

            puts("[ MOUSETEST ] Mousetest beendet!");

            return -1;
        }
    }
    
    FILE *mouse;
    mouse = fopen("kbc:/mouse/data", "r");
    setvbuf(stdout, malloc(128), _IOLBF, 128);
    char mousedata[12];

    float x = 320;
    float y = 240;
    uint8_t old_buttons = 0;

    bool change = true;

    printf("\nAktuelle Mausposition: (Beenden mit beliebiger Taste)\n");
    while (true)
    {
        char c;
        if (fread(&c, 1, 1, stdin)) {
            break;
        }

    	if (fread(mousedata, 12, 1, mouse))
    	{
            if (((int*) mousedata)[0] || ((int*) mousedata)[1]) {
                change = true;
            }

            if (old_buttons != mousedata[8]) {
                change = true;
            }

    		x += ((int*)mousedata)[0];
    		y += ((int*)mousedata)[1];
    		if (x > 640) x = 640;
    		if (x < 0) x = 0;
    		if (y > 480) y = 480;
    		if (y < 0) y = 0;

            if (change) {
                change = false;

                printf("\r %3d/%3d, ", (int)x, (int)y);
                int i;

                old_buttons = mousedata[8];
                uint8_t buttons = old_buttons;
                for (i = 0; i < 8; i++)
                {
                    printf("%c", (buttons & 0x01)?('1'):('0'));
                    buttons = buttons >> 1;
                }

                fflush(stdout);
            }
    	}

        yield();
    }

    printf("\n");
    return 0;
}
