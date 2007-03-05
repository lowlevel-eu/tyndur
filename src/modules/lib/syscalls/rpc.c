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
    dword rounded_len = len;
    if((len % 4) != 0)
    {
        rounded_len += 4 - (len % 4);
    }
    
    //printf("send_message(%d, %d, %d, %d(%d), 0x%x);\n", pid, function, correlation_id, len, rounded_len, (dword) data);

    asm(
        "movl %4, %%eax;"
        "add %5, %%eax;"
        "movl %5, %%ecx;"
        ".1: sub $4, %%eax;"
        "pushl (%%eax);"
        "sub $4, %%ecx;"
        "jg .1;"
        "pushl %3;"
        "pushl %2;"
        "pushl %1;"
        "pushl %0;"
        "mov $51, %%eax;"
        "int $0x30;"
        "add $0x10, %%esp;"
        "add %5, %%esp;"
    : : "g" (pid), "g" (len+8), "g"(function), "g" (correlation_id), "g" (data) , "g" (rounded_len) : "eax", "ecx");
}
