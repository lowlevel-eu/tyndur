#include "types.h"

extern int puts(const char* s);
__volatile__ void rpc_handler();

extern void set_rpc_handler(void* rpc_handler);

void _start() {
    puts("\nSetze RPC-Handler...");
    set_rpc_handler(&rpc_handler);
    
    while(1);
}

void rpc_handler() {
    puts("RPC-Handler\n");

    // Übergebene Daten vom Stack räumen
    //asm("addl $0x8, %esp;");
}
