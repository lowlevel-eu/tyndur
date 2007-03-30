#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"

#define _USE_START_
#include "init.h"
#include "lostio.h"

FILE* stdout;
FILE* stderr;

int main(int argc, char* argv[])
{
    int i = 0;
    while(1)
    {
        if (stdout == NULL)
        {
            stdout = fopen("console:/stdout","w");
        } 
        
        if (stderr == NULL)
        {
            stderr = fopen("console:/stderr","w"); 
        }
        
        if (stdout != NULL && stderr != NULL)
        {
            if (i++ % 2 == 1)
            {
                fprintf(stdout,"Hello World. \n\0");
            }
            else
            {
                fprintf(stderr,"Example Error. \n\0");
            }
        }
    }
    printf("After While\n");
    while(1) yield();
}


