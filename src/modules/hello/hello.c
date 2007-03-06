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
    char* buffer = malloc(1024);
    
    
    /*
    printf("Lese von pci:/\n\n");

    io_res = fopen("pci:/", "w");
    while((read_bytes = fread(buffer, 128, 1, io_res))) {
        if (read_bytes < 128) {
            buffer[read_bytes + 1] = '\0';
        } else {
            if (read_bytes > 128) {
                printf("BUG! Rueckgabe von fread zu gross.\n");
            }
            buffer[128] = '\0';
        }
        printf("%s", buffer);
    }
    fclose(io_res);

    printf("\nFertig.\n");
    return 0;
    

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
  
    io_res = fopen("dma:/1/68/1024", "r");*/
    

/*    io_res = fopen("floppy:/0", "rw");
    byte* buffer = malloc(512);
    if(io_res != NULL)
    {
        fread(buffer, 512, 1, io_res);
        printf("Lese von der Diskette: %x%x\n", buffer[1], buffer[0]);
    

        buffer[1] = 0x13;
        buffer[0] = 0x37;
        printf("Schreibe auf die Diskette: %x%x\n", buffer[1], buffer[0]);
        fwrite(buffer, 512, 1, io_res);

        fread(buffer, 512, 1, io_res);
        printf("Lese von der Diskette: %x%x\n", buffer[1], buffer[0]);
        fclose(io_res);

        io_res = fopen("floppy:/version", "rw");
        fread(buffer, 1, 512, io_res);
        printf("Version des Diskettentreibers:'%s'\n", (char*) buffer);
        fclose(io_res);
    }
    else
    {
        puts("Konnte floppy:/devices/0 nicht oeffnen");
    }
    */
    
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
    
    
    
    io_res = fopen("floppy:/version", "rw");
    fread(buffer, 1, 512, io_res);
    printf("fread von floppy:/version:'%s'\n", (char*) buffer);
    
    printf("Teste fwrite auf ein Ramfile\n");
    char* write_test = "fwrite auf ein Ramfile funktioniert!";
    fwrite(write_test, 1, strlen(write_test) + 1, io_res);

    fread(buffer, 1, 512, io_res);
    printf("fread: Version:'%s'\n", (char*) buffer);
    fclose(io_res);

    return 0;
    
}

