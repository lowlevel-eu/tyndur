; startup
extern PASCALMAIN
global _start

section .text

_start:
        call PASCALMAIN
        hlt
        
section .data
        
section .bss
