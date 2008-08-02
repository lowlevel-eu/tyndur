#include <types.h>
#include <config.h>

#include "cpu.h"
#include "syscall.h"
#include "syscallno.h"
#include "kprintf.h"
#include "kernel.h"

#define DEBUG_SYSCALLS 1

#if 0
// Neues Syscall-Interface
void syscall_arch(machine_state_t* isf)
{
    dword* stack = (dword*) isf->esp;
    dword number = *stack++;
    dword arg_count = *stack++;

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
    dword result;
    dword stack_backup;
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
    dword* stack = (dword*) isf->esp;
    dword number = isf->eax;
    //kprintf("Syscall %d", number);

    if ((number >= SYSCALL_MAX) || (syscalls[number].handler == NULL)) {
        // FIXME: Hier sollte nur der Prozess beendet werden
        panic("Ungueltiger Syscall %d", number);
    }

    void* handler = syscalls[number].handler;

    // FIXME Das ist alles nur bedingt ueberzeugend...
    if (handler == syscall_fastrpc) {

        pid_t callee_pid = *((dword*) isf->esp);
        dword metadata_size = *((dword*) (isf->esp + 4));
        void* metadata = *((void**) (isf->esp + 8));
        dword data_size = *((dword*) (isf->esp + 12));
        void* data = *((void**) (isf->esp + 16));

        isf->eax = syscall_fastrpc(
            callee_pid,
            metadata_size, metadata,
            data_size, data);

    } else if (handler == syscall_fastrpc_ret) {

        syscall_fastrpc_ret();

    } else {
        dword result;
        dword stack_backup;

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
}

