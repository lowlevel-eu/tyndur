#include "types.h"
#include "syscall.h"

void rpc(dword pid) 
{
    dword result = 0;

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

void send_message
    (dword pid, dword function, dword correlation_id, dword len, char* data)
{
    dword result = 0;

    // Auf die Variable wird zwar sonst nirgends schreibend zugegriffen, aber
    // das volatile ist noetig, weil gcc sonst der Meinung ist, der Arrayinhalt
    // wuerde nicht benutzt (dem asm-Block wird nur der Pointer uebergeben).
    // Ohne volatile laesst gcc mit -O1 die Initialisierung des Arrays daher
    // grosszuegig unter den Tisch fallen.
    volatile dword metadata[2] = { function, correlation_id };
    
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
        : "=a" (result) 
        : "i" (SYSCALL_FASTRPC), "g" (pid), "i" (0x8), "g" (metadata), 
            "g" (len), "g" (data), "0" (result));

    } while (result != 0);
}

