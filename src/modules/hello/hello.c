#include "types.h"
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "rpc.h"

#define _USE_START_
#include "init.h"




int main(int argc, char* argv[])
{
    yield();
    
    printf("get dword from HELOWRLD RPC = %d\n", rpc_get_dword(init_service_get("rpcrecv"), "HELOWRLD", 0, NULL));

    return -50;
}

