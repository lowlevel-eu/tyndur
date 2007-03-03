#include "types.h"
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "rpc.h"
#include "io.h"

#define _USE_START_
#include "init.h"


int main(int argc, char* argv[])
{
    yield();
    yield();
    yield();
    yield();

    FILE* io_res;
    /*const char* hw = "Hello world!";
    char* file_content = malloc(0x20);
    
    io_res = fopen("init_test:/", "w");
    fwrite(hw, 1, strlen(hw) + 1, io_res);
    fread(file_content, 1, 0x20, io_res);
    fclose(io_res);

    printf("Oeffne und lese 'init_test:/': `%s`\n", file_content);



    io_res = fopen("init_test:/|rot:/13", "w");
    fwrite(hw, 1, strlen(hw) + 1, io_res);
    fread(file_content, 1, 0x20, io_res);
    fclose(io_res);

    printf("Oeffne und lese 'init_test:/|rot:/13': `%s`\n", file_content);




    io_res = fopen("init_test:/|rot:/12", "w");
    fwrite(hw, 1, strlen(hw) + 1, io_res);
    fread(file_content, 1, 0x20, io_res);
    fclose(io_res);

    printf("Oeffne und lese 'init_test:/|rot:/12': `%s`\n", file_content);
  
    io_res = fopen("dma:/1/68/1024", "r");*/

    io_res = fopen("floppy:/0", "rw");
    byte* buffer = malloc(512);
    fread(buffer, 512, 1, io_res);
    printf("Lese von der Diskette: %x%x\n", buffer[1], buffer[0]);
    

    buffer[1] = 0x13;
    buffer[0] = 0x37;
    printf("Schreibe auf die Diskette: %x%x\n", buffer[1], buffer[0]);
    fwrite(buffer, 512, 1, io_res);

    fread(buffer, 512, 1, io_res);
    printf("Lese von der Diskette: %x%x\n", buffer[1], buffer[0]);
    fclose(io_res);
    return 0;
}

