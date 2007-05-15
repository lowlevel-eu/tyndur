#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"

#define _USE_START_
#include "init.h"


int main(int argc, char* argv[])
{
    puts("[SKELETON] Module loaded.");
    
    while(1) {
        wait_for_rpc();
    }
}


