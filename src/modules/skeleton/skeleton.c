#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"


int main(int argc, char* argv[])
{
    puts("[SKELETON] Module loaded.");
    
    while(1) {
        wait_for_rpc();
    }
}


