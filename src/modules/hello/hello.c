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
    
    //printf("get dword from HELOWRLD RPC = %d\n", rpc_get_dword(init_service_get("rpcrecv"), "HELOWRLD", 0, NULL));
 
    
    FILE* io_res = fopen("init_test", "w");

    const char* hw = "Hello world!";
    fwrite(hw, 1, strlen(hw) + 1, io_res);

    fseek(io_res, 1, SEEK_SET);
    fwrite("a", 1, 1, io_res);
    fseek(io_res, 0, SEEK_SET);

    char* file_content = malloc(0x20);
    fread(file_content, 1, 0x20, io_res);
    
    puts(file_content);
    
    fclose(io_res);
    return -50;
}







