/*
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Antoine Kaufmann.
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

.code32
.section .text

    // Multiboot header
    .align 4
    .int 0x1BADB002
    .int 0x03
    .int -(0x1BADB002 + 0x03)

    .globl _start
    _start:
        // Ein bisschen Stack schadet nie
        lea stack_space, %esp
        
        // Als erstes wird der Bildschirm sauber gemacht, damit potentielle
        // Fehlermeldungen gut sichtbar sind
        pushl %eax
        movl $(2 * 80 * 25), %ecx
        xorb %al, %al
        movl $0xB8000, %edi
        rep stosb
        popl %eax
       
        // Ueberpruefen ob ein Multiboot-Bootloader benutzt wurde
        cmp $0x2BADB002, %eax
        jne error_no_multiboot

        // Multiboot infos retten
        movl %ebx, (multiboot_info)

        // Erst muss mal geprueft werden, ob der Longmode ueberhaupt zur
        // Verfuegung steht. Das geschieht mit CPUID
        mov $0x80000001, %eax
        cpuid
        bt $29, %edx
        jnc error_no_longmode
        
        // Hier werden die ganzen Tabellen fuers Paging vorbereitet. Was hier
        // gemacht wird ist nur temporaer. Hier werden nur die ersten 1GB
        // identity mapped.

        // Alle Tabellen fuer Paging auf 0 setzen
        movl $0x3000, %ecx
        lea paging_table_lvl4, %edi
        movb $0, %al
        rep stosb
        
        // Den Eintrag in der Level4-Tabelle machen
        lea paging_table_lvl3, %eax
        orl $7, %eax
        movl %eax, paging_table_lvl4

        // Den Eintrag in der Level3-Tabelle machen
        lea paging_table_lvl2, %eax
        orl $7, %eax
        movl %eax, paging_table_lvl3
    
        // Jetzt wird das Pagedirectory vorbereitet
        movl $0x87, %eax
        lea paging_table_lvl2, %edx
        movl $512, %ecx
    
    pagedir_loop:
        movl %eax, (%edx)
        addl $8, %edx
        addl $(2 * 1024 * 1024), %eax
        loop pagedir_loop
        
        // PAE aktivieren
        movl %cr4, %eax
        orl $0x20, %eax
        movl %eax, %cr4

        // Adresse der Level4-Tabelle in cr3 laden
        lea paging_table_lvl4, %eax
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
        lgdt gdtr
        jmp  $0x8, $longmode

    .code64
    longmode:
        xorq %rdi, %rdi
        movl multiboot_info, %edi

        .extern loader
        call loader

        hlt
    

    .code32
    // Hier wird bei Fehlern hingesprungen. Eine Fehlermeldung wird ausgegeben,
    // danach wird angehalten.
    error_no_multiboot:
        lea error_msg_no_multiboot, %esi
        jmp error

    error_no_longmode:
        lea error_msg_no_longmode, %esi

    error:
        movl $0xB8000, %edi

    .1:
        lodsb
        
        test %al,%al
        jz .end
        
        stosb
        movb $0x07, %al
        stosb
        jmp .1
    .end:
        
        hlt



.section .data
    error_msg_no_multiboot:
        .ascii "Im Moment wird ein Modultiboot-Bootloader vorausgesetzt"
        .byte 0

    error_msg_no_longmode: 
        .ascii "Longmode nicht verfuegbar"
        .byte 0

    // Hier wird die Adresse der Multibootinfos gelagert
    multiboot_info: .int 0

    // Die GDT besteht nur aus 3 Deskriptoren: Null, Code und Daten
    gdt:
        // Der Nulldeskriptor
        .quad 0

        // Der Code-Deskriptor
        .int 0
        .byte 0
        .byte 0x98
        .byte 0x20
        .byte 0

        // Der Daten-Deskriptor:
        .int 0
        .byte 0
        .byte 0x90
        .byte 0
        .byte 0



    // Struktur um die GDT laden zu koennen
    gdtr:
        // Wir haben 3 Eintraege
        .word 3 * 8
        .quad gdt

.section .bss

    // Tabellen fuer Paging
    .align 4096
    paging_table_lvl4: .space 4096
    paging_table_lvl3: .space 4096
    paging_table_lvl2: .space 4096
    
    // 4Kb fuer den Stack reservieren.
    .space 4096
    stack_space:




