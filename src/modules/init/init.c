#include "types.h"
#include "syscall.h"
#include "rpc.h"

void test1(dword data_size, void* data);
void test2(dword data_size, void* data);

void _start() {
    puts("\nSetze RPC-Handler für INIT...");
    init_messaging();

    if (!register_message_handler("TEST___1", &test1)) {
        puts("Konnte RPC-Handler nicht registrieren\n");
    }
    if (!register_message_handler("TEST___2", &test2)) {
        puts("Konnte RPC-Handler nicht registrieren\n");
    }
    
    while(1);
}

void test1(dword data_size, void* data)
{
    puts("[RPCRECV] Hello world (aus TEST1)!\n");
}

void test2(dword data_size, void* data)
{
    puts("[RPCRECV] Hello world (aus TEST2)!\n");
}
