#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "init.h"

void hello_world(pid_t pid, dword correlation_id, size_t data_size, void* data);

void _start() {
    v();
    init_memory_manager();
    puts("\nSetze RPC-Handler...");
    init_messaging();

    init_service_register("rpcrecv");



    if (!register_message_handler("HELOWRLD", &hello_world)) {
        puts("Konnte RPC-Handler nicht registrieren\n");
    }

    while(1) yield();
}

void hello_world(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
   printf("[RPCRECV] Hello, world! pid=%d  correlation_id=%d\n", pid, correlation_id);

    rpc_send_dword_response(pid, correlation_id, 7654);
}
