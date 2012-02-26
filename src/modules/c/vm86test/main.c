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

#include <stdio.h>
#include <syscall.h>
#include <stdint.h>
#include <string.h>

#include <init.h>

typedef struct vesa_info_t
{
    char vesa[4];
    uint16_t version;
    uint32_t vendorinfo[2];
    uint32_t videomodes;
    uint16_t memory;
} __attribute__ ((packed)) vesa_info_t;

int main(int argc, char* argv[])
{
    printf("VGA: Lese derzeitigen Videomodus aus.\n");
	  vm86_regs_t regs;
	  regs.ax = 0x0F00;
    printf("Rufe BIOS-Funktion auf.\n");
    vm86_int(&regs, 0);
    printf("BIOS-Funktion beendet.\n");
    printf("Modus: %d Spalten, Modus %d, Page %d.\n\n", regs.ax >> 8, regs.ax & 0xFF, regs.bx >> 8);
    
    printf("Lese VESA-Informationen aus.\n");
	  regs.ax = 0x4F00;
	  regs.es = 0x8000;
	  regs.ds = 0x8000;
	  regs.di = 0x0000;
	  char vesadata[512];
	  memcpy(vesadata, "VBE2", 4);
	  uint32_t meminfo[4];
	  meminfo[0] = 1;
	  meminfo[1] = 0x80000;
	  meminfo[2] = (uint32_t)vesadata;
	  meminfo[3] = 512;
    vm86_int(&regs, meminfo);
	  /*regs.ax = 0x4F00;
	  regs.es = 0x8000;
	  regs.ds = 0x8000;
	  regs.di = 0x0000;
	  memcpy(vesadata, "VBE2", 4);
	  meminfo[0] = 1;
	  meminfo[1] = 0x80000;
	  meminfo[2] = vesadata;
	  meminfo[3] = 512;
    vm86_int(&regs, meminfo);*/
    printf("VESA: \"%s\"\n", vesadata);
    printf("ax: %x\n", regs.ax);
    printf("bx: %x\n", regs.bx);
    printf("cx: %x\n", regs.cx);
    printf("dx: %x\n", regs.dx);
    printf("si: %x\n", regs.si);
    printf("di: %x\n", regs.di);
    
    vesa_info_t *info = (vesa_info_t*)vesadata;
    printf("Memory: %d\n", info->memory * 64 * 1024);
    printf("Modelist: %X\n", info->videomodes);
    
    while(1);
    return 1;
}

