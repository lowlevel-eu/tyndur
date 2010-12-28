; startup
extern PASCALMAIN

section .text

global _start
_start:
        mov [__ppb_shm_id], eax
        mov [__ppb_ptr], ebx
        mov [__ppb_size], ecx
        call PASCALMAIN
        hlt

section .data

section .bss

global __ppb_size
global __ppb_ptr
global __ppb_shm_id

__ppb_size:     resd 1
__ppb_ptr:      resd 1
__ppb_shm_id:   resd 1
