#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"

void hello_world(pid_t pid, dword correlation_id, size_t data_size, void* data);

void _start() {
    puts("\nSetze RPC-Handler...");
    init_messaging();
    
    if (!register_message_handler("HELOWRLD", &hello_world)) {
        puts("Konnte RPC-Handler nicht registrieren\n");
    }
    
    while(1);
}

void hello_world(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    puts("[RPCRECV] Hello, world!\n");

    rpc_send_dword_response(pid, correlation_id, 7654);
}
