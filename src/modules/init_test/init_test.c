#include "types.h"
#include "syscall.h"
#include "stdlib.h"
#include "string.h"




#define INIT_PID 1
#define INIT_SERVICE_REGISTER "SERVICE_REGISTER"
void service_register(const char* name);




void putn(unsigned long long x, int radix, int pad, char padchar);

void service_register(const char* name)
{
    const char* rpc_name = INIT_SERVICE_REGISTER;
    char* msg = malloc(strlen(rpc_name) + strlen(name) + 1);
    puts("1");
    strncpy(msg, rpc_name, strlen(rpc_name));
    puts("2");
    strncpy((char*)((dword)msg + strlen(rpc_name)), name, strlen(name) + 1);
    puts("3");
    send_message(INIT_PID, 512, 0, strlen(msg) + 1, msg);
    puts("4");
}

void _start() {
    // Damit init erst mal 
    yield();
    init_messaging();
    init_memory_manager();
    
    puts("ok\n\033[1;33mRegistriere Server beim init Prozess...\033[0;37m");
    service_register("init_test");
    

    while(1) yield();
}

