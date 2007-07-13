#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "config.h"

#define BLOCK_SIZE 1024

void cat_display_usage();

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_cat(int argc, char* argv[], const char* args)
#else
    #define _USE_START_
    #include "init.h"
    int main(int argc, char* argv[])
#endif
{
    if (argc != 2) {
        cat_display_usage();
        return -1;
    }
    
    char* path = argv[1];
    FILE* file = fopen(path, "r");

    if (file == NULL) {
        printf("Konnte '%s' nicht zum lesen oeffnen!\n", path);
    } else {
        char buffer[BLOCK_SIZE + 1];
        size_t size;
        
        while (feof(file) != EOF) {
            size = fread(buffer, 1, BLOCK_SIZE, file);
            buffer[size] = 0;

            printf("%s", buffer);
        }
        
        fclose(file);
        printf("\n");
    }

    return EXIT_SUCCESS;
}

void cat_display_usage()
{
    puts("\nAufruf: cat <Dateiname>");
}

