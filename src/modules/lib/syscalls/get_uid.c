#include "types.h"
#include "syscall.h"

dword get_uid(dword pid) {
    dword uid;

    asm(
        "pushl %2;"
        "mov %1, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
    : "=a" (uid) : "i" (SYSCALL_PM_GET_UID), "r" (pid));

    return uid;
}
