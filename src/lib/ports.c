/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Damian Bodde.
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

/*
 * Vorläufige Funktionen zur Verwaltung von outb, outw, outl # inb, inw, inl
*/
#include "types.h"
#include "ports.h"

word inw(word _port)
{
	word result;
	__asm__ ("inw %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}

/// in in byte ///
byte inb(word _port)
{
	byte result;
	__asm__ ("inb %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}

/// in in long(32 bit) ///
dword inl(word _port)
{
	dword result;
	__asm__ ("inl %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}



/// out in dword ///
void outw(word _port, word _data)
{
	__asm__ ("outw %0, %1" : : "a" (_data), "Nd" (_port));
}

/// out in byte ///
void outb(word _port, byte _data)
{
	__asm__ ("outb %0, %1" : : "a" (_data), "Nd" (_port));
}

/// out in long(32 bit) ///
void outl(word _port, dword _data)
{
	__asm__ ("outl %0, %1" : : "a"(_data), "Nd" (_port));
}

/* Ein Byte an einen IO Port senden und für langsame Ports kurz verzögern */
void outb_wait(word _port, byte _data)
{
	__asm__ ("outb %0, %1\njmp 1f\n1: jmp 1f\n1:" : : "a" (_data), "Nd" (_port));
}
