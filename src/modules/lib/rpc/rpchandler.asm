section .text

global rpc_handler
extern c_rpc_handler

rpc_handler:
    pusha
    
    mov eax, [esp + 32]
    lea edx, [esp + 36]

    push edx
    push eax
    call c_rpc_handler
    add esp, 8

    popa

    ; Hier ist die eigentliche Arbeit getan. Jetzt muß nur noch der Stack
    ; in den richtigen Zustand gebracht werden (und die Register müssen
    ; so bleiben, wie sie sind)
    push eax
    mov eax, [esp+4]
    add esp, eax
    add esp, 8
    neg eax
    mov eax, [esp + eax - 8]
    ret
