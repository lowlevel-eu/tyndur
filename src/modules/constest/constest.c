#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"

#define _USE_START_
#include "init.h"
#include "lostio.h"


int main(int argc, char* argv[])
{
    puts("[SKELETON] Module loaded.\n");
    int i = 0;
    for(i = 0; i < 10; yield(i++));
    for(i = 0; i < 10; yield(i++));
    while(1)
    {
        printf("Open, try %d\n",++i);
        fopen("console\:\/create/hello_world","r");
        printf("Open, try %d (2nd file)\n",i);
        fopen("console\:\/create/second","r");
        fopen("console\:\/create","r");
    }
    printf("After While\n");
    while(1) yield();
}


