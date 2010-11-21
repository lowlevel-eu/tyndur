.code32

.globl vm86_task_entry
.extern vm86_get_backlink_pointer
vm86_task_entry:
    call vm86_get_backlink_pointer
    mov %bx, (%eax)

    pushfl
    pop %eax
    or $0x4000, %eax
    push %eax
    popfl

    iret
