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

global librpc_rpc_handler
extern librpc_c_rpc_handler

librpc_rpc_handler:
    pusha
    
    mov eax, [esp + 32]
    mov ecx, [esp + 36]
    lea edx, [esp + 40]

    push edx
    push eax
    push ecx
    call librpc_c_rpc_handler
    add esp, 12
    
    popa

    mov eax, 56
    int 0x30

    ; Hierher darf er nicht kommen => GPF verursachen
    hlt

    ; Hier ist die eigentliche Arbeit getan. Jetzt muß nur noch der Stack
    ; in den richtigen Zustand gebracht werden (und die Register müssen
    ; so bleiben, wie sie sind)
    
    ; Das Original-eax auf den Stack wegsichern
    push eax

    ; esp + 4 = data_size
    ; Falls data_size nicht durch vier teilbar ist, aufrunden
    mov eax, [esp+4]
    and eax, 0x3
    jz .1
    mov eax, [esp+4]
    or eax, 0x3
    inc eax
    jmp .2
    .1:
    mov eax, [esp+4]
    .2:

    ; Stack verkleinern und das Original-eax zurückholen
    add esp, eax
    add esp, 12
    neg eax
    mov eax, [esp + eax - 12]


    ;push eax
    ; Selber Workaround wie oben, EFLAGS wiederherstellen
    ;mov eax, 54
    ;int 0x30 
    ;pop eax

    ret
