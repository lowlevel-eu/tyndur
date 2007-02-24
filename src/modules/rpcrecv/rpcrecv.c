#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"

#define _USE_START_
#include "init.h"


void hello_world(pid_t pid, dword correlation_id, size_t data_size, void* data);

int main(int argc, char* argv[])
{
    v();
    init_memory_manager();
    puts("\nSetze RPC-Handler...");
    init_messaging();
    init_sync_messages();    

    init_service_register("rpcrecv");



    if (!register_message_handler("HELOWRLD", &hello_world)) {
        puts("Konnte RPC-Handler nicht registrieren\n");
    }

    while(1) yield();
}

void hello_world(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    rpc_send_dword_response(pid, correlation_id, 1337);
}
