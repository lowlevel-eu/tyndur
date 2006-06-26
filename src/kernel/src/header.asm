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
; 2006-06-05: PorkChicken
;   ! Workarounds gegen das Problem mit den Unterstrichen vor Symbolen
;
; 2006-06-16: PorkChicken
;   ! Workarounds wieder entfernt, da jetzt ein Cross-Compiler Pflicht ist


extern init
global _start

section .text
_start:
	mov esp, 0x90000
	mov ebp, esp

	; Die Differenz zwischen Link-Adresse und Ladeadresse berechnen
	call $+5
	pop edx
	sub edx, $-1

	; Eine temporäre GDT erstellen
	mov word [ebp], 0x0027 ; limit
	mov [ebp + 2], ebp ; base

	; 0x08: 4gb code selector, base 0
	mov dword [ebp + 0x08], 0x0000ffff
	mov dword [ebp + 0x0c], 0x00cf9a00

	; 0x10: 4gb data selector, base 0
	mov dword [ebp + 0x10], 0x0000ffff
	mov dword [ebp + 0x14], 0x00cf9200

	; 0x18: 4gb code selector, base edx
	; 0x20: 4gb data selector, base edx
	mov ecx, edx
	shl ecx, 16
	or ecx, 0x0000ffff
	mov dword [ebp + 0x18], ecx
	mov dword [ebp + 0x20], ecx
	mov ecx, edx
	shr ecx, 16
	xchg cl, ch
	ror ecx, 8
	or ecx, 0x00cf9a00
	mov dword [ebp + 0x1c], ecx
	xor ecx, 0x00cf9200 ^ 0x00cf9a00
	mov dword [ebp + 0x24], ecx

	; Temporäre GDT laden
	lgdt [ebp]
	jmp dword 0x18:$+7
	mov cx, 0x0020
	mov ds, cx
	mov es, cx
	mov ss, cx
	sub ebp, edx
	mov esp, ebp

	push edx
	push ebx
	push eax
	call init
	hlt

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
