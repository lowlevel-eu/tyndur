;
; Copyright (c) 2007 The tyndur Project. All rights reserved.
;
; This code is derived from software contributed to the tyndur Project
; by Antoine Kaufmann.
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
[bits 16]
; Wird vom Kernel dorthin geladen
org 0x7000

; Hier starten die neuen Prozessoren ihre Arbeit

; Der Kernel schreibt die Adresse, an die die Prozessoren springen sollen,
; sobald sie fertig initialisiert sind, an einen Offset von 2 Bytes
entry:
    jmp start

; Diese Speicherstelle wird vom Kernel gesetzt
kernel_entry: dd 0

start:
    ; GDT-Laden
    lgdt[gdtr]

    ; Protected-Mode aktivieren
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; FAR-Jump ins Codesegment
    jmp 0x8:protected_mode

[bits 32]
protected_mode:
    ; Segmente laden
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Einspungspunkt fuer den Kernel laden
    mov eax, [kernel_entry]
    jmp eax


; Das GDT-Register wird zum laden der GDT benutzt
gdtr:
    dw 24
    dd gdt
    
; Unsere GDT. Sie besteht nur aus 2 Deskriptoren (3 mit Null-Deskriptor)
; 1. Code von 0 - 4GB mit PL0
; 2. Data von 0 - 4GB mit PL0
gdt:
        ; Null-Deskriptor
        dw 0x0000
        dw 0x0000
        dw 0x0000
        dw 0x0000
        
        ; Code-Deskriptor
        dw 0xFFFF
        dw 0x0000
        dw 0x9800
        dw 0x00CF

        ; Daten-Deskriptor
        dw 0xFFFF
        dw 0x0000
        dw 0x9200
        dw 0x00CF

