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
.code32

// Wird vom SMP-Init auf die Adresse der Pagemaps gesetzt.
.global smp_entry_context
smp_entry_context: .int 0

smp_entry_lock: .byte 0

// Hier betreten die zusaetzlichen Prozessoren den Kernel
.globl smp_pm_entry
smp_pm_entry:
    // Die Prozessoren sollen alle einzeln die Initialisierung durchlaufen
    // Das vereinfacht das ganze ein wenig, da so nur ein temporaerer Stack
    // benoetigt wird.
    movb $1, %bl
smp_entry_lock_loop:
    xorb %al, %al
    lock cmpxchgb %bl, smp_entry_lock
    jnz smp_entry_lock_loop
    
    
    // PAE aktivieren
    movl %cr4, %eax
    orl $0x20, %eax
    movl %eax, %cr4
  
    // Die Adresse der Pagemap in cr3 laden
    movl smp_entry_context, %eax
    movl %eax, %cr3

    // Hier wird der der Longmode aktivert. Das geschieht durch das setzen
    // des Longmode enable Bits im EFER-MSR.
    movl $0xC0000080, %ecx
    rdmsr
    orl $0x100, %eax
    wrmsr

    // Die eigentliche Aktivierung des Longmodes wird aber erst durch das
    // aktivieren von Paging vollzogen:
    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0
        
    // Die neue GDT laden
    .extern gdtr
    lgdt gdtr
    jmp  $0x18, $smp_entry_longmode

.code64
smp_entry_longmode:
    // Den temporaeren Stack benoetigen wir, damit die pmm_alloc-Funktion
    // aufgerufen werden kann, um einen sauberen Stack fuer die einzelnen
    // Prozessoren zu allozieren.
    lea smp_tmp_stack, %rsp
    //movq %rsp, %rbp
    // Eine Page
    movq $1, %rdi
    
    .extern pmm_alloc
    call pmm_alloc

    // Da der Stack von oben nach unten waechst, wird der Stack-Pointer auf das
    // obere Ende der Page gerichtet.
    addq $0x1000, %rax
    movq %rax, %rsp

    // Siehe oben
    push $0
    push $0
    movq %rsp, %rbp
    
    // Den Lock wieder aufheben
    movl $0, smp_entry_lock

    // Das ist nicht der Bootstrap Processor bzw. Core
    xorl %edx, %edx

    // multiboot_magic und multiboot_info werden nicht gebraucht
    xorl %edi, %edi
    xorl %esi, %esi

    .extern init
    call init

    // Geschieht nie
    hlt

.section .bss

// Wird fuer den Stack waehrend der SMP-Initialisierung benutzt, solange noch
// keine eigenen Pages dafuer alloziert sind.
    .space 256
smp_tmp_stack:

