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
; 2006-07-12: Jidder
;   ! via Paging (statt Segmentation) den Kernel nach 0xc0000000 mappen
;   ! das log wird mir zu lang, deswegen hab ich mein Zeug mal zensiert ;)
;

; Nur ein paar Flags werden hier definiert. Das entgültige Paging wird erst später initialisiert.
%define PAGE_SUPERVISOR		(0 << 2)
%define PAGE_READWRITE		(1 << 1)
%define PAGE_NOT_PRESENT	(0 << 0)
%define PAGE_PRESENT		(1 << 0)

%define PAGE_SHIFT			12 ; um eine Adresse in eine Page umzurechnen, muss sie um 12 Stellen nach rechts geschoben werden
%define PGDIR_SHIFT			20 ; um eine Adresse einen Page Directory Eintrag zuzuordnen, muss sie um 20 Stellen nach rechts geschoben werden

%define VIRT_TO_PHYS_ADDR 0xc0000000 ; diesen Wert von einer virtuellen Adresse subtrahieren, um die physische zu erhalten

extern init
global _start

section .text
_start:

	; Stack initalisieren
	mov esp, kernelstack - VIRT_TO_PHYS_ADDR
	mov ebp, esp

	; Die vom Multiboot Loader übergebenen Informationen auf den Stack legen (init benötigt sie)
	push ebx
	push eax

	; Page Directory Initialisieren
	mov edi, page_directory
	mov ecx, 1024
	mov eax, PAGE_SUPERVISOR | PAGE_READWRITE | PAGE_NOT_PRESENT
	rep stosd

	; Eine Page Table initalisieren, die die ersten 4 MBytes mappt
	xor eax, eax
	mov edi, page_table0 - VIRT_TO_PHYS_ADDR
	call fill_page_table

	; die ersten 4 MBytes nach 0MB und VIRT_TO_PHYS_ADDR mappen
	mov eax, page_table0 - VIRT_TO_PHYS_ADDR
	or eax, PAGE_SUPERVISOR | PAGE_READWRITE | PAGE_PRESENT
	mov dword [page_directory - VIRT_TO_PHYS_ADDR], eax
	mov dword [page_directory - VIRT_TO_PHYS_ADDR + (VIRT_TO_PHYS_ADDR >> PGDIR_SHIFT)], eax 

	; von 4 MB bis 20 MB identity mappen
	mov eax, 0x00400000
	mov edi, page_table1 - VIRT_TO_PHYS_ADDR
	call fill_page_table
	mov eax, page_table1 - VIRT_TO_PHYS_ADDR
	or eax, PAGE_SUPERVISOR | PAGE_READWRITE | PAGE_PRESENT
	mov dword [page_directory - VIRT_TO_PHYS_ADDR + (0x00400000 >> PGDIR_SHIFT)], eax

	mov eax, 0x00800000
	mov edi, page_table2 - VIRT_TO_PHYS_ADDR
	call fill_page_table
	mov eax, page_table2 - VIRT_TO_PHYS_ADDR
	or eax, PAGE_SUPERVISOR | PAGE_READWRITE | PAGE_PRESENT
	mov dword [page_directory - VIRT_TO_PHYS_ADDR + (0x00800000 >> PGDIR_SHIFT)], eax

	mov eax, 0x00c00000
	mov edi, page_table3 - VIRT_TO_PHYS_ADDR
	call fill_page_table
	mov eax, page_table3 - VIRT_TO_PHYS_ADDR
	or eax, PAGE_SUPERVISOR | PAGE_READWRITE | PAGE_PRESENT
	mov dword [page_directory - VIRT_TO_PHYS_ADDR + (0x00c00000 >> PGDIR_SHIFT)], eax

	mov eax, 0x01000000
	mov edi, page_table4 - VIRT_TO_PHYS_ADDR
	call fill_page_table
	mov eax, page_table4 - VIRT_TO_PHYS_ADDR
	or eax, PAGE_SUPERVISOR | PAGE_READWRITE | PAGE_PRESENT
	mov dword [page_directory - VIRT_TO_PHYS_ADDR + (0x01000000 >> PGDIR_SHIFT)], eax

	; Paging aktivieren
	mov eax, page_directory - VIRT_TO_PHYS_ADDR
	mov cr3, eax
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

	; Stack in den hohen Speicher verlegen
	add esp, VIRT_TO_PHYS_ADDR
	add ebp, VIRT_TO_PHYS_ADDR

	mov eax, init
	call eax
	hlt

fill_page_table:
; Füllt eine Page Table, die 4 MBytes durchgehend mappt.
; in:
;   eax -> erste Page, die gemappt werden soll
;   edi -> Page Table
; clobbers:
;   eax, ecx, edi
	mov ecx, 1024
.page_table_fill_next
	or eax, PAGE_SUPERVISOR | PAGE_READWRITE | PAGE_PRESENT
	stosd
	add eax, 4096
	loop .page_table_fill_next
	ret


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

	page_directory resb 4096
	page_table0 resb 4096
	page_table1 resb 4096
	page_table2 resb 4096
	page_table3 resb 4096
	page_table4 resb 4096
