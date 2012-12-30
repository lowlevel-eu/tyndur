/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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
        movl %ebx, multiboot_info
        
        // Erst muss mal geprueft werden, ob der Longmode ueberhaupt zur
        // Verfuegung steht. Das geschieht mit CPUID
        mov $0x80000001, %eax
        cpuid
        bt $29, %edx
        jnc error_no_longmode
        
        /*.extern _binary_rm_trampoline_lin_o_start
        .extern _binary_rm_trampoline_lin_o_end
        lea _binary_rm_trampoline_lin_o_start, %esi
        lea _binary_rm_trampoline_lin_o_end, %ecx
        subl $_binary_rm_trampoline_lin_o_start, %ecx
        movl $0x7000, %edi
    .copy_trampoline:
        lodsb
        stosb
        loop .copy_trampoline
        */
        
        
        // Hier werden die ganzen Tabellen fuers Paging vorbereitet. Was hier
        // gemacht wird ist nur temporaer. Hier werden nur die ersten 1GB
        // identity mapped.

        // Alle Tabellen fuer Paging auf 0 setzen
        movl $0x6000, %ecx
        lea paging_table_lvl4, %edi
        movb $0, %al
        rep stosb
        
        // Den Eintrag in der Level4-Tabelle machen
        lea paging_table_lvl3, %eax
        orl $7, %eax
        lea paging_table_lvl4, %ebx
        movl %eax, (%ebx)
        movl %eax, 0xFF8(%ebx)

        // Die Adressen der 4 Pagedirectories eintragen
        lea paging_table_lvl2, %eax
        lea paging_table_lvl3, %edx
        orl $7, %eax

        movl $4, %ecx
    pagedir_ptr_loop:
        movl %eax, (%edx)
        addl $4096, %eax
        addl $8, %edx
        loop pagedir_ptr_loop
    
        // Jetzt werden 4 Pagedirectories vorbereitet
        movl $0x197, %eax
        lea paging_table_lvl2, %edx
        movl $2048, %ecx
    
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
        
        // Caching deaktivieren, da das beim initialisieren gehoerig schief
        // gehen kann.
        orl $0x10, %eax

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
        lgdt init_gdtr
        jmp  $0x18, $longmode

    .code64
    longmode:
        /*movw $0x10, %ax
        movw %ax, %ds
        movw %ax, %es
        movw %ax, %ss*/

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
        .ascii "Im Moment wird ein Multiboot-Bootloader vorausgesetzt"
        .byte 0

    error_msg_no_longmode: 
        .ascii "Longmode nicht verfuegbar"
        .byte 0

    // Hier wird die Adresse der Multibootinfos gelagert
    multiboot_info: .int 0
    

.section .bss

    // Tabellen fuer Paging
    .align 0x1000
    paging_table_lvl4: .space 0x1000
    .align 0x1000
    paging_table_lvl3: .space 0x1000
    .align 0x1000
    paging_table_lvl2: .space 0x1000 * 4
    
    // 4Kb fuer den Stack reservieren.
    .space 4096
    stack_space:

.section .data
    // Die GDT besteht nur aus 4 Deskriptoren: Null, 32-Bit-Code, Daten und
    // 64-Bit-Code
    init_gdt:
        // Null-Deskriptor
        .quad 0

        // 32-Bit-Code-Deskriptor
        .word 0xFFFF
        .word 0x0000
        .byte 0x00
        .byte 0x98
        .byte 0xCF
        .byte 0x00

        // Daten-Deskriptor
        .word 0xFFFF
        .word 0x0000
        .byte 0x00
        .byte 0x92
        .byte 0xCF
        .byte 0x00

        // 64-Bit-Code-Deskriptor
        .int 0
        .byte 0
        .byte 0x98
        .byte 0x20
        .byte 0

    init_gdtr:
        .word 4 * 8
        .int init_gdt


