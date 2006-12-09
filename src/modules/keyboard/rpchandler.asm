section .text

global rpc_handler
extern irq_handler

rpc_handler:
    pusha
    call irq_handler
    popa
    push eax
    mov eax, [esp+4]
    add esp, eax
    add esp, 8
    neg eax
    mov eax, [esp + eax - 8]
    ret
