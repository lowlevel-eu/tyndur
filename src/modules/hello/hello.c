#include "types.h"
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "rpc.h"
#include "io.h"

#define _USE_START_
#include "init.h"


/*
FILE* fopen(const char* filename, const char* mode);
size_t fread(void* dest, size_t blocksize, size_t blockcount, FILE* io_res);
size_t fwrite(const void* src, size_t blocksize, size_t blockcount, FILE* io_res);
int fseek (FILE* io_res, long int offset, int origin);
*/

int main(int argc, char* argv[])
{
    yield();
    yield();
    yield();
    yield();

    FILE* io_res;
    const char* hw = "Hello world!";
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
    
    fclose(io_res);
    return -50;
}


