#include <stdint.h>
#include <lost/config.h>

#include "cpu.h"
#include "syscall.h"
#include "syscallno.h"
#include "kprintf.h"
#include "kernel.h"
#include "debug.h"
#include "tasks.h"

#define DEBUG_SYSCALLS 1

#if CONFIG_ARCH == ARCH_I386
void increase_user_stack_size(pm_thread_t* task_ptr, int pages);
#endif

#if 0
// Neues Syscall-Interface
void syscall_arch(machine_state_t* isf)
{
    uint32_t* stack = (uint32_t*) isf->esp;
    uint32_t number = *stack++;
    uint32_t arg_count = *stack++;

#ifdef DEBUG_SYSCALLS
    kprintf("Syscall %d mit %d Argumenten\n", number, arg_count);
    int i = 0;
    for(i = 0; i < arg_count; i++) {
        kprintf(" - %d: 0x%x\n", i, stack[i]);
    }
#endif
    
    if ((number >= SYSCALL_MAX) || (syscalls[number].handler == NULL)) {
        // FIXME: Hier sollte nur der Prozess beendet werden
        panic("Ungueltiger Syscall %d", number);
    }

    if (syscalls[number].arg_count != arg_count) {
        // FIXME: s.o.
        panic("Ungueltige Argumentanzahl fuer Syscall %d, verlangt werden %d "
                "uebergeben wurden %d", number, syscalls[number].arg_count, arg_count);
    }

    void* handler = syscalls[number].handler;
    uint32_t result;
    uint32_t stack_backup;
    // Die Syscallhandler werden mit dem Userspace-Stack aufgerufen, damit
    // keine Parameter kopiert werden muessen.
    asm("pusha;"
        // Den original-Stackpointer retten
        "movl %%esp, %2;"
        // Stack wechseln
        "movl %1, %%esp;"
        "call *%3;"
        "movl %%eax, %0;"
        "movl %2, %%esp;"
        "popa;"
        : "=m" (result) : "r" (stack), "m" (stack_backup), "r" (handler));
    
    isf->eax = result;
}
#endif
// "Kompatibel" zu kernel1
void syscall_arch(machine_state_t* isf)
{
    uint32_t* stack;
    uint32_t number = isf->eax;
    //kprintf("Syscall %d", number);

    if ((number >= SYSCALL_MAX) || (syscalls[number].handler == NULL)) {
        // FIXME: Hier sollte nur der Prozess beendet werden
        panic("Ungueltiger Syscall %d", number);
    }

    void* handler = syscalls[number].handler;
    if (isf_is_userspace(isf)) {
        if (syscalls[number].privileged) {
            // FIXME: Hier sollte nur der Prozess beendet werden
            panic("Privilegierter Syscall %d", number);
        }
        stack = (uint32_t*) isf->esp;
    } else {
        /* esp wird bei Interrupt in Ring 0 nicht gepusht, das ist also der
         * erste "alte" Stackinhalt */
        stack = (uint32_t*) &isf->esp;
    }

    if (debug_test_flag(DEBUG_FLAG_SYSCALL)) {
        kprintf("[PID %d] Syscall:%d\n", current_process->pid, isf->eax);
        io_ports_check(current_process);
    }

    // FIXME Das ist alles nur bedingt ueberzeugend...
    if (handler == syscall_fastrpc) {

        pid_t callee_pid = *((uint32_t*) isf->esp);
        uint32_t metadata_size = *((uint32_t*) (isf->esp + 4));
        void* metadata = *((void**) (isf->esp + 8));
        uint32_t data_size = *((uint32_t*) (isf->esp + 12));
        void* data = *((void**) (isf->esp + 16));

        isf->eax = syscall_fastrpc(
            callee_pid,
            metadata_size, metadata,
            data_size, data);

    } else if (handler == syscall_fastrpc_ret) {

        syscall_fastrpc_ret();

    } else if (handler == syscall_mem_info) {

        syscall_mem_info(&isf->eax, &isf->edx);

    } else {
        uint32_t eax, edx;

        asm volatile(
            "mov %%esp, %%ebx;"
            "sub %%ecx, %%esp;"
            "shr $0x2, %%ecx;"
            "mov %%esp, %%edi;"
            "rep movsd;"
            "call *%4;"
            "mov %%ebx, %%esp;"
            : "=a" (eax), "=d" (edx), "=S" (stack)
            : "S" (stack), "r" (handler), "c" (syscalls[number].arg_count * 4)
            : "edi", "ebx", "memory");

        isf->eax = eax;
        if (handler == syscall_get_tick_count) {
            isf->edx = edx;
        }
    }
}

