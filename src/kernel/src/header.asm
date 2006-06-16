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
  mov   esp, kernelstack
  push  ebx
  push  eax
  call  init
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
