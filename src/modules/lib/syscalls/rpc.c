#include "types.h"
#include "syscall.h"

void rpc(dword pid) 
{
    dword result;

    do {
        asm(
            "pushl $0;"
            "pushl %1;"
            "mov $51, %%eax;"
            "int $0x30;"
            "add $0x8, %%esp;"
        : "=a" (result) : "g" (pid), "0" (result));
    } while (result != 0);
}

void signal(dword pid, dword signal) 
{
    dword result;

    do {
        asm(
            "pushl %2;"
            "pushl $4;"
            "pushl %1;"
            "mov $51, %%eax;"
            "int $0x30;"
            "add $0xb, %%esp;"
        : "=a" (result) : "g" (pid), "g" (signal + 256), "0" (result));
    } while (result != 0);
}

void send_message(dword pid, dword function, dword correlation_id, dword len, char* data)
{
    dword result;
    dword metadata[2] = { function, correlation_id };
    
    do {
        asm(
            "pushl %6;"
            "pushl %5;"
            "pushl %4;"
            "pushl %3;"
            "pushl %2;"
            "mov %1, %%eax;"
            "int $0x30;"
            "add $0x14, %%esp;"
        : "=a" (result) : "i" (SYSCALL_FASTRPC), "g" (pid), "i" (0x8), "g" (metadata), "g" (len), "g" (data), "0" (result));
    } while (result != 0);
}
