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

exception_stub 0
exception_stub 1
exception_stub 2
exception_stub 3
exception_stub 4
exception_stub 5
exception_stub 6
exception_stub 7
exception_stub 8
exception_stub 9
exception_stub 10
exception_stub 11
exception_stub 12
exception_stub 13
exception_stub 14
exception_stub 16
exception_stub 17
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