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
    } while (!result);
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
    } while (!result);
}

void send_message(dword pid, dword function, dword correlation_id, dword len, char* data)
{
    dword result;

    dword rounded_len = len;
    if((len % 4) != 0)
    {
        rounded_len += 4 - (len % 4);
    }
    
    //printf("send_message(%d, %d, %d, %d(%d), 0x%x);\n", pid, function, correlation_id, len, rounded_len, (dword) data);    
    do {
        asm volatile (
            // eax = data + rounded_len (Obere Grenze)
            // ecx = rounded_len
            "movl %5, %%eax;"
            "add %6, %%eax;"
            "movl %6, %%ecx;"
            
            // Daten auf den Stack pushen:
            // while ecx != 0 do
            //     push *eax
            //     ecx -= 4
            // done
            "or %%ecx, %%ecx;"
            "jz .2;"
            ".1: sub $4, %%eax;"
            "pushl (%%eax);"
            "sub $4, %%ecx;"
            "jg .1;"

            // Syscall aufrufen
            ".2: pushl %4;"
            "pushl %3;"
            "pushl %2;"
            "pushl %1;"
            "mov $51, %%eax;"
            "int $0x30;"
            "add $0x10, %%esp;"

            // Daten vom Stack räumen
            "add %6, %%esp;"
        : "=a" (result) : "g" (pid), "g" (len+8), "g"(function), "g" (correlation_id), "g" (data) , "g" (rounded_len), "0" (result) : "ecx");
    } while (result != 0);
}
