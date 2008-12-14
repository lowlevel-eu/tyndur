/*
 Copyright (c) 2007 The tyndur Project. All rights reserved.

 This code is derived from software contributed to the tyndur Project
 by Antoine Kaufmann.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
 3. All advertising materials mentioning features or use of this software
    must display the following acknowledgement:
    This product includes software developed by the tyndur Project
    and its contributors.
 4. Neither the name of the tyndur Project nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Initialisiert esp und ruft main() auf

.extern init
.globl _start

.section .text
_start:
    lgdt gdtr64
	// Stack initalisieren
	lea kernelstack, %rsp

	// Damit die Stack Traces hier enden ein Stack Frame mit Nullwerten für die 
    // Rücksprungadresse und den alten Stack Frame erstellen
	push $0
	push $0
	movq %rsp, %rbp

	// Die vom Multiboot Loader übergebenen Informationen auf den Stack legen 
    // (init benötigt sie)
    xorq %rsi, %rsi
	movl %ebx, %esi
    xorq %rdi, %rdi
	movl %eax, %edi

	lea init, %rax
	call *%rax
	cli
	hlt

// Dieser Speicher wird als Stack waehrend der Initialisierung benutzt
    .space  16384
kernelstack:

.section .data
    gdt64:
        // Der Nulldeskriptor
        .word 0x0000
        .word 0x0000
        .word 0x0000
        .word 0x0000

        // Der Code-Deskriptor
        .word 0x0000
        .word 0x0000
        .word 0x9800
        .word 0x0020

        // Der Daten-Deskriptor:
        .word 0x0000
        .word 0x0000
        .word 0x9000
        .word 0x0000

    // Struktur um die GDT laden zu koennen
    .globl gdtr64
    gdtr64:
        // Wir haben 3 Eintraege
        .word 3 * 8
        .quad gdt64


