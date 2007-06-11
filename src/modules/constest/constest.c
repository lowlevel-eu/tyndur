#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"

#define _USE_START_
#include "init.h"
#include "lostio.h"
char stdout_msg[] = "Hello World.\n";
char stderr_msg[] = "Example Error.\n";

typedef enum { STDIN, STDOUT, STDERR } stdioe;

typedef struct
{
  pid_t pid;
  stdioe type;
} console_ctrl_t;

void init_stdio()
{
    while((stdout = fopen("console:/stdout","a")) == NULL)
    {
        printf("[ CONSTEST ] Open for stdout FAILED.\n");
    }

    while((stderr = fopen("console:/stderr","a")) == NULL)
    {
        printf("[ CONSTEST ] Open for stderr FAILED.\n");
    }
}



void rpc_set_console(enum { STDIN, STDOUT, STDERR} type, char* path)
{
    pid_t console = init_service_get("console");
    if (console)
    {
        void* control = malloc(strlen(path)+sizeof(console_ctrl_t));
        ((console_ctrl_t*)control)->pid = -1;
        ((console_ctrl_t*)control)->type = type;
        strncpy((char*)control + sizeof(console_ctrl_t),path,strlen(path));
        rpc_get_int(console,"CONS_SET",strlen(path)+sizeof(console_ctrl_t),control);
        free(control);
    }
    else
    {
        printf("[ CONSTEST ] Fehler: Kann console nicht finden!\n");
    }
}

int main(int argc, char* argv[])
{
    init_stdio();
    rpc_set_console(STDOUT,"vterm:/term5");
    int i = 0;

    while(i < 20)
    {
        
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
    fwrite("After While\n",20,1,stdout);
    return 0;
}


