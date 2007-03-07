#include "types.h"
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "rpc.h"
#include "io.h"
#include "dirent.h"
#define _USE_START_
#include "init.h"


int main(int argc, char* argv[])
{
    int i;
    for(i = 0; i < 50; i++)
    {
        yield();
    }

    FILE* io_res;
    char* buffer = malloc(512);
    
    
    dword read_bytes; 
    printf("Lese von pci:/\n\n");

    io_res = fopen("pci:/", "r");

    while((read_bytes = fread(buffer, 128, 1, io_res))) {
        if (read_bytes < 128) {
            buffer[read_bytes] = '\0';
        } else {
            if (read_bytes > 128) {
                printf("BUG! Rueckgabe von fread zu gross: %d.\n", read_bytes);
            }
            buffer[128] = '\0';
        }
        printf("%s", buffer);
    }
    fclose(io_res);

    printf("\nFertig.\n");
    
    

    
    
    printf("Lese floppy:/ aus:\n");
    io_resource_t* dir_res = directory_open("floppy:/");
    
    if(dir_res != NULL)
    {
        io_direntry_t* direntry;
        
        while((direntry = directory_read(dir_res)))
        {
            if(direntry->type == IO_DIRENTRY_FILE)
            {
                printf("   %d   ", (int) direntry->size);
                printf("      %s\n", direntry->name);
            }
            else
            {
                printf("   %d   ", (int) direntry->size);
                printf("[DIR] %s\n", direntry->name);
            }
            
        }

        directory_close(dir_res);
    }
    else
    {
        puts("Konnte floppy:/ nicht oeffnen");
    }
    
    return 0;
    
}

