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

char stdout_msg[] = "Hello World.\n";
char stderr_msg[] = "Example Error.\n";

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
        //        fprintf(stdout,"Hello World. \n\0");
                fwrite(stdout_msg,20,1,stdout);
            }
            else
            {
        //        fprintf(stderr,"Example Error. \n\0");
                fwrite(stderr_msg,20,1,stderr);
            }
        }
    }
    printf("After While\n");
    while(1) yield();
}


