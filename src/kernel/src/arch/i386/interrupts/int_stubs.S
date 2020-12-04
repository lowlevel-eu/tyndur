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


// Fuer jeden Interrupt wird ein eigener Einsprungspunkt definiert. Dabei
// existieren zwei verschiedene Arten von Handlern: Solche fuer Exceptions,
// die einen Fehlercode auf dem Stack haben, und solche fuer die uebrigen.
// Die Stubs bringen den Stack in einen einheitlichen Zustand, speichern die
// Interruptnummer auf den Stack und springen dann zu einem gemeinsamen
// Handler.

// Dieser Stub ist fuer Interrupts ohne Fehlercode. Als Fehlercode wird in
// diesem Fall 0 auf den Stack gelegt.
.macro INT_STUB nr
    .globl im_int_stub_\nr
    im_int_stub_\nr:
    pushl $0
    pushl $\nr
    jmp im_int_stub_all
.endm

// Dieser Stub ist fuer Interrupts, die bereits einen Fehlercode auf dem Stack
// haben. In diesem Fall wird nur die Interruptnummer gespeichert.
.macro INT_STUB_ERROR_CODE nr
    .globl im_int_stub_\nr
    im_int_stub_\nr:
    pushl $\nr
    jmp im_int_stub_all
.endm

// Definition der Stubs fuer jeden Interrupt
INT_STUB 0
INT_STUB 1
INT_STUB 2
INT_STUB 3
INT_STUB 4
INT_STUB 5
INT_STUB 6
INT_STUB 7
// Double Fault (8) wird gesondert behandelt
INT_STUB 9
INT_STUB_ERROR_CODE 10
INT_STUB_ERROR_CODE 11
INT_STUB_ERROR_CODE 12
INT_STUB_ERROR_CODE 13
INT_STUB_ERROR_CODE 14
INT_STUB 15
INT_STUB 16
INT_STUB_ERROR_CODE 17
INT_STUB 18
INT_STUB 19
INT_STUB 20
INT_STUB 21
INT_STUB 22
INT_STUB 23
INT_STUB 24
INT_STUB 25
INT_STUB 26
INT_STUB 27
INT_STUB 28
INT_STUB 29
INT_STUB 30
INT_STUB 31
INT_STUB 32
INT_STUB 33
INT_STUB 34
INT_STUB 35
INT_STUB 36
INT_STUB 37
INT_STUB 38
INT_STUB 39
INT_STUB 40
INT_STUB 41
INT_STUB 42
INT_STUB 43
INT_STUB 44
INT_STUB 45
INT_STUB 46
INT_STUB 47
INT_STUB 48
INT_STUB 49
INT_STUB 50
INT_STUB 51
INT_STUB 52
INT_STUB 53
INT_STUB 54
INT_STUB 55
INT_STUB 56
INT_STUB 57
INT_STUB 58
INT_STUB 59
INT_STUB 60
INT_STUB 61
INT_STUB 62
INT_STUB 63
INT_STUB 64
INT_STUB 65
INT_STUB 66
INT_STUB 67
INT_STUB 68
INT_STUB 69
INT_STUB 70
INT_STUB 71
INT_STUB 72
INT_STUB 73
INT_STUB 74
INT_STUB 75
INT_STUB 76
INT_STUB 77
INT_STUB 78
INT_STUB 79
INT_STUB 80
INT_STUB 81
INT_STUB 82
INT_STUB 83
INT_STUB 84
INT_STUB 85
INT_STUB 86
INT_STUB 87
INT_STUB 88
INT_STUB 89
INT_STUB 90
INT_STUB 91
INT_STUB 92
INT_STUB 93
INT_STUB 94
INT_STUB 95
INT_STUB 96
INT_STUB 97
INT_STUB 98
INT_STUB 99
INT_STUB 100
INT_STUB 101
INT_STUB 102
INT_STUB 103
INT_STUB 104
INT_STUB 105
INT_STUB 106
INT_STUB 107
INT_STUB 108
INT_STUB 109
INT_STUB 110
INT_STUB 111
INT_STUB 112
INT_STUB 113
INT_STUB 114
INT_STUB 115
INT_STUB 116
INT_STUB 117
INT_STUB 118
INT_STUB 119
INT_STUB 120
INT_STUB 121
INT_STUB 122
INT_STUB 123
INT_STUB 124
INT_STUB 125
INT_STUB 126
INT_STUB 127
INT_STUB 128
INT_STUB 129
INT_STUB 130
INT_STUB 131
INT_STUB 132
INT_STUB 133
INT_STUB 134
INT_STUB 135
INT_STUB 136
INT_STUB 137
INT_STUB 138
INT_STUB 139
INT_STUB 140
INT_STUB 141
INT_STUB 142
INT_STUB 143
INT_STUB 144
INT_STUB 145
INT_STUB 146
INT_STUB 147
INT_STUB 148
INT_STUB 149
INT_STUB 150
INT_STUB 151
INT_STUB 152
INT_STUB 153
INT_STUB 154
INT_STUB 155
INT_STUB 156
INT_STUB 157
INT_STUB 158
INT_STUB 159
INT_STUB 160
INT_STUB 161
INT_STUB 162
INT_STUB 163
INT_STUB 164
INT_STUB 165
INT_STUB 166
INT_STUB 167
INT_STUB 168
INT_STUB 169
INT_STUB 170
INT_STUB 171
INT_STUB 172
INT_STUB 173
INT_STUB 174
INT_STUB 175
INT_STUB 176
INT_STUB 177
INT_STUB 178
INT_STUB 179
INT_STUB 180
INT_STUB 181
INT_STUB 182
INT_STUB 183
INT_STUB 184
INT_STUB 185
INT_STUB 186
INT_STUB 187
INT_STUB 188
INT_STUB 189
INT_STUB 190
INT_STUB 191
INT_STUB 192
INT_STUB 193
INT_STUB 194
INT_STUB 195
INT_STUB 196
INT_STUB 197
INT_STUB 198
INT_STUB 199
INT_STUB 200
INT_STUB 201
INT_STUB 202
INT_STUB 203
INT_STUB 204
INT_STUB 205
INT_STUB 206
INT_STUB 207
INT_STUB 208
INT_STUB 209
INT_STUB 210
INT_STUB 211
INT_STUB 212
INT_STUB 213
INT_STUB 214
INT_STUB 215
INT_STUB 216
INT_STUB 217
INT_STUB 218
INT_STUB 219
INT_STUB 220
INT_STUB 221
INT_STUB 222
INT_STUB 223
INT_STUB 224
INT_STUB 225
INT_STUB 226
INT_STUB 227
INT_STUB 228
INT_STUB 229
INT_STUB 230
INT_STUB 231
INT_STUB 232
INT_STUB 233
INT_STUB 234
INT_STUB 235
INT_STUB 236
INT_STUB 237
INT_STUB 238
INT_STUB 239
INT_STUB 240
INT_STUB 241
INT_STUB 242
INT_STUB 243
INT_STUB 244
INT_STUB 245
INT_STUB 246
INT_STUB 247
INT_STUB 248
INT_STUB 249
INT_STUB 250
INT_STUB 251
INT_STUB 252
INT_STUB 253
INT_STUB 254
INT_STUB 255

// Hier ist der Beginn des gemeinsamen Handlercodes fuer alle Interrupts.
//
// Wir haben an dieser Stelle im grossen und ganzen noch den Prozessorzustand
// des Prozesses, der durch den Interrupt unterbrochen wurde. Der Stackpointer
// des Prozesses wurde vom Prozessor ins TSS gespeichert und dafuer der dort
// gespeicherte Kernelstack geladen.
//
// Auf dem Kernelstack liegen jetzt die Ruecksprungdaten (vom Prozessor beim
// Interrupt dort abgelegt - bei einem Interrupt aus Ring 3 sind das ss, esp,
// eflags, cs und eip), der Fehlercode und die Interruptnummer.
//
// Der unterbrochene Task weiss nicht, dass ein Interrupt stattgefunden hat,
// deswegen muss beim Ruecksprung der gesamte Prozessorzustand
// wiederhergestellt werden. Bevor wir den Kernel oder andere Tasks Register
// veraendern lassen, muessen wir also erst einmal den aktuellen Zustand
// speichern. Wir benutzen dazu den Stack, der ja schon dem Kernel gehoert.

im_int_stub_all:
    // Das rax-Register wird benoetigt, um ds und es Pushen zu koennen
    pushl %eax

    // Segmentregister auf den Stack pushen
    push %gs
    push %fs

    xorl %eax, %eax
    movw %es, %ax
    pushl %eax

    movw %ds, %ax
    pushl %eax

    // Allzweckregister auf den Stack
    pushl %ebp
    pushl %edi
    pushl %esi
    pushl %edx
    pushl %ecx
    pushl %ebx

    // Der Task-Zustand ist hier fertig gespeichert.

    // Ring 0 Segment-Register laden
    cld
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es


    // Der aktuelle Stackpointer wird dem C-Interrupt-Handler als Parameter
    // uebergeben. Wir haben gerade den Zustand des Gasts im Format des C-Typs
    // interrupt_stack_frame_t auf den Stack gepusht, so dass der C-Handler
    // bequem auf die einzelnen Register des Tasks zugreifen kann.
    pushl %esp

    .extern im_handler
    call im_handler

    // Diese Sprungmarke wird beim Initialisieren des Kernels zum Starten der
    // ersten Threads benoetigt
.globl im_run_thread
im_run_thread:

    // Der C-Handler gibt einen neuen Stackpointer fuer den Kernelstack des
    // aktuellen Tasks zurueck (der aktuelle Task kann sich geaendert haben,
    // wenn wir gerade einen Timer-Interrupt oder einen Syscall verarbeiten).
    //
    // Den neuen Stackpointer laden. Anschliessend liegt der Zustand des jetzt
    // aktuellen Tasks auf dem Stack, den wir auslesen koennen. Anschliessend
    // koennen wir in den Task zurueckspringen.
    movl %eax, %esp

    // Allzweckregister wiederherstellen
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ebp

    // Segmentregister wiederherstellen
    popl %eax
    movw %ax, %ds

    popl %eax
    movw %ax, %es

    pop %fs
    pop %gs

    // eax wiederherstellen
    popl %eax

    // Fehler- und Interruptnummer von Stack loeschen
    addl $8, %esp

    // Ruecksprung in den Task
    iret



// Double Fault
.globl im_int_stub_8
.extern double_fault_tss
im_int_stub_8:

    // Paging ausschalten
    movl %cr0, %eax
    andl $0x8000000, %eax
    movl %eax, %cr0

    // Fehlermeldung
    push $msg_double_fault
    call kprintf

    // CPU-Dump
    // Das passende TSS wird in ebx uebergeben

    pushl    8(%ebx) /* ss */
    pushl  1*4(%ebx) /* esp */
    pushl  9*4(%ebx) /* eflags */
    pushl 19*4(%ebx) /* cs */
    pushl  8*4(%ebx) /* eip */

    pushl $0x0 /* Fehlercode */
    pushl $0x8 /* Interruptnummer */

    pushl 10*4(%ebx) /* eax */

    pushl 23*4(%ebx) /* gs */
    pushl 22*4(%ebx) /* fs */
    pushl 18*4(%ebx) /* es */
    pushl 21*4(%ebx) /* ds */

    pushl 15*4(%ebx) /* ebp */
    pushl 17*4(%ebx) /* edi */
    pushl 16*4(%ebx) /* esi */
    pushl 12*4(%ebx) /* edx */
    pushl 11*4(%ebx) /* ecx */
    pushl 13*4(%ebx) /* ebx */

    pushl %esp
    call cpu_dump

    cli
    hlt
    jmp .

msg_double_fault:
    .ascii "\033[1;41mPANIC: Double fault\n\0"

