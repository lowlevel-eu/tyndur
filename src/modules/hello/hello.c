#include "types.h"
#include "syscall.h"
#include "stdlib.h"

void putn(unsigned long long x, int radix, int pad, char padchar);

void _start() {
    char *hw = "Hello world!";
    dword ptr;

    printf("ok\n\033[1;33mAllokiere Speicher mit malloc...\033[0;37m\n\n");
    
    init_memory_manager();
    int n;
    //for (n = 0;; n++) malloc(n);//mem_allocate(n, 0);//malloc(n);
    
    char *test = malloc(24);
    memcpy(test, "Starte Stresstest\0", 18);
    printf(test);
   
    char *testpointer[20];
    int i = 0;
    for(;;)
    {
        //printf("start malloc\n\n");
        for(i=1; i < 20; i++)
            testpointer[i] = malloc(i*1024);
        //printf("start freeing\n\n");
        for(i=1; i < 20; i++)
            free(testpointer[i]);
    }

    

    
    asm(
        "push %0;"
        "xor %%eax, %%eax;"
        "int $0x30;"
        "add $0x4, %%esp;"
        : : "r" (hw));


    puts("ok\n\033[1;33mFuehre RPC durch...\033[0;37m\n");
    for (i = 0; i < 5; i++) {
        send_message(1, 512, 0, 8, "HELOWRLD");
    }
    /*
    asm(
        "pushl $0;"
        "pushl $0;"
        "mov $51, %eax;"
        "int $0x30;"
        "add $0x8, %esp;"
        );
    }*/
    
    asm(
        "mov $42, %eax;"
        "int $0x30;");

    while(1);
}
