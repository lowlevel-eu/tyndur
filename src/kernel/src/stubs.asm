;
; Copyright (c) 2006 The tyndur Project. All rights reserved.
;
; This code is derived from software contributed to the tyndur Project
; by Burkhard Weseloh.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
; 3. All advertising materials mentioning features or use of this software
;    must display the following acknowledgement:
;     This product includes software developed by the tyndur Project
;     and its contributors.
; 4. Neither the name of the tyndur Project nor the names of its
;    contributors may be used to endorse or promote products derived
;    from this software without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
; ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
; TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
; PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
; CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
; EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
; ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;

section .text

%define IRQ_BASE 0x20

extern handle_int

%macro exception_stub 1
global exception_stub_%1
exception_stub_%1:
	push dword 0
	push dword %1
	jmp int_bottom
%endmacro

%macro exception_stub_error_code 1
global exception_stub_%1
exception_stub_%1:
	push dword %1
	jmp int_bottom
%endmacro

%macro irq_stub 1
global irq_stub_%1
irq_stub_%1:
	push dword 0
	push dword %1 + IRQ_BASE
	jmp int_bottom
%endmacro

global null_handler
null_handler:
	push dword 0
	push dword 0x1337
	jmp int_bottom

global syscall_stub
syscall_stub:
  push dword 0
  push dword 0x30
	jmp int_bottom

exception_stub 0
exception_stub 1
exception_stub 2
exception_stub 3
exception_stub 4
exception_stub 5
exception_stub 6
exception_stub 7
exception_stub_error_code 8
exception_stub 9
exception_stub_error_code 10
exception_stub_error_code 11
exception_stub_error_code 12
exception_stub_error_code 13
exception_stub_error_code 14
exception_stub 16
exception_stub_error_code 17
exception_stub 18
exception_stub 19

irq_stub 0
irq_stub 1
irq_stub 2
irq_stub 3
irq_stub 4
irq_stub 5
irq_stub 6
irq_stub 7
irq_stub 8
irq_stub 9
irq_stub 10
irq_stub 11
irq_stub 12
irq_stub 13
irq_stub 14
irq_stub 15

int_bottom:
	; register sichern
	pusha
	push ds
	push es
	push fs
	push gs

	; ring 0 segment register laden
	cld
	mov ax, 0x10
	mov ds, ax
	mov es, ax

	; c-handler aufrufen
	push esp
	call handle_int
	add esp, 4

	; den stack wechseln
	mov esp, eax
	
	; register laden
	pop gs
	pop fs
	pop es
	pop ds
	popa

	add esp, 8 ; fehlercode und interrupt nummer überspringen

	iret
