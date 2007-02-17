#include "types.h"
#include "syscall.h"
#include "stdlib.h"
#include "string.h"




#define INIT_PID 1
void service_register(const char* name);




void putn(unsigned long long x, int radix, int pad, char padchar);
void service_register(const char* name);



void _start() {
    init_messaging();
    init_memory_manager();
    yield();
    puts("\n\033[1;33mRegistriere Server beim init Prozess...\033[0;37m");
    send_message(INIT_PID, 512, 0, 10, "SERV_REGbl");
    puts("ok");
    service_register("init_test");
    

    while(1) yield();
}


void service_register(const char* name)
{
    const char* rpc_name = "SERV_REG";
    char* msg = malloc(strlen(rpc_name) + strlen(name) + 1);
    
    memcpy(msg, rpc_name, strlen(rpc_name));
    memcpy((char*)((dword)msg + strlen(rpc_name)), name, strlen(name));
    
    msg[strlen(name) + strlen(rpc_name)] = (char)0;
    
    send_message(INIT_PID, 512, 0, strlen(msg) + 1, msg);
}
