#include "types.h"
#include "syscall.h"

void rpc(dword pid) 
{
    asm(
        "pushl $0;"
        "pushl %0;"
        "mov $51, %%eax;"
        "int $0x30;"
        "add $0x8, %%esp;"
    : : "g" (pid));
}

void signal(dword pid, dword signal) 
{
    asm(
        "pushl %1;"
        "pushl $4;"
        "pushl %0;"
        "mov $51, %%eax;"
        "int $0x30;"
        "add $0xb, %%esp;"
    : : "g" (pid), "g" (signal + 256));
}

void send_message(dword pid, dword function, dword correlation_id, dword len, char* data)
{

    len += 8;
    asm(
        "movl %4, %%eax;"
        "add %%eax, %1;"
        "movl %1, %%ecx;"
        ".1: push (%%eax);" // TODO: Wird das richtig als pushb übersetzt?
        "dec %%eax;"
        "dec %%ecx;"
        "jnz .1;"
        "pushl %3;"
        "pushl %2;"
        "pushl %1;"
        "pushl %0;"
        "mov $51, %%eax;"
        "int $0x30;"
        "add $0x10, %%esp;"
        "add %1, %%esp;"
    : : "g" (pid), "g" (len), "g"(function), "g" (correlation_id), "g" (data) : "eax", "ecx");
}
