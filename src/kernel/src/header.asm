;
; Copyright (c) 2006 The tyndur Project. All rights reserved.
;
; This code is derived from software contributed to the tyndur Project
; by Kevin Wolf.
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

; Initialisiert esp und ruft main() auf
;
; Aenderungen:
;
; 2006-04-09: taljeth
;   + Erste Version erstellt
;
; 2006-04-09: Golum
;   + Uebergabe der von GRUB bereitgestellten Informationen
;
; 2006-04-13: nore
;   ! falsch gesetztes "kernelstack" Label an die richtige Stelle gesetzt
;
; 2006-07-12: Jidder
;   ! via Paging (statt Segmentation) den Kernel nach 0xc0000000 mappen
;   ! das log wird mir zu lang, deswegen hab ich mein Zeug mal zensiert ;)
;

extern init
global _start

section .text
_start:

	; Stack initalisieren
	mov esp, kernelstack

	; Damit die Stack Traces hier enden ein Stack Frame mit Nullwerten für die Rücksprungadresse und den alten Stack Frame erstellen
	push 0
	push 0
	mov ebp, esp

	; Die vom Multiboot Loader übergebenen Informationen auf den Stack legen (init benötigt sie)
	push ebx
	push eax

	mov eax, init
	call eax
	cli
	hlt

section multiboot
multiboot_header:
align 4
  MULTIBOOT_MAGIC     equ 0x1BADB002
  MULTIBOOT_FLAGS     equ 0x03
  MULTIBOOT_CHECKSUM  equ -MULTIBOOT_MAGIC-MULTIBOOT_FLAGS

  dd MULTIBOOT_MAGIC
  dd MULTIBOOT_FLAGS
  dd MULTIBOOT_CHECKSUM

section .bss
  resb  16384
kernelstack:
