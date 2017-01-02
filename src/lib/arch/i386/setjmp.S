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

.global setjmp
.type	setjmp, @function
setjmp:
    pushl %ebp
    movl %esp, %ebp

    pushl %edi
    movl 8(%ebp), %edi

    //edi = Sprungdaten (jmp_buf)
    //Sichere Register

    movl %eax, 0(%edi)
    movl %ebx, 4(%edi)
    movl %ecx, 8(%edi)
    movl %edx, 12(%edi)
    movl %esi, 16(%edi)

    //Sichere edi
    movl -4(%ebp), %eax
    movl %eax, 20(%edi)

    //Sichere Rest des Stack Frames
    //Vorheriger ebp
    movl 0(%ebp), %eax
    movl %eax, 24(%edi)
    //Beginn des Stack Frames
    movl %esp, %eax
    addl $12, %eax
    movl %eax, 28(%edi)
    //Rücksprungaddresse
    movl 4(%ebp), %eax
    movl %eax, 32(%edi)

    popl %edi
    movl $0, %eax
    leave
    ret
.size setjmp, .-setjmp

.global longjmp
.type	longjmp, @function
longjmp:
    pushl %ebp
    movl %esp, %ebp

    //edi = Sprungdaten (jmp_buf)
    movl 8(%ebp), %edi
    //Zurückgabewert in die Sprungdaten schreiben
    movl 12(%ebp), %eax
    movl %eax, 0(%edi)

    //Register wiederherstellen
    movl 24(%edi), %ebp

    movl 28(%edi), %esp

    pushl 32(%edi)

    movl 0(%edi), %eax
    movl 4(%edi), %ebx
    movl 8(%edi), %ecx
    movl 12(%edi), %edx
    movl 16(%edi), %esi
    movl 20(%edi), %edi

    ret
.size longjmp, .-longjmp
