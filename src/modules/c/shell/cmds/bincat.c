#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "config.h"

#define BLOCK_SIZE 1024

void bincat_display_usage();

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_bincat(int argc, char* argv[], const char* args)
#else
    #define _USE_START_
    #include "init.h"
    int main(int argc, char* argv[])
#endif
{
    if (argc != 2) {
        bincat_display_usage();
        return -1;
    }

    char* path = argv[1];
    FILE* file = fopen(path, "r");

    if (file == NULL) {
        printf("Konnte '%s' nicht zum lesen oeffnen!\n", path);
        return -1;
    } else {
        char buffer[BLOCK_SIZE];
        size_t size;
        int n, m = 0;
        while (feof(file) != EOF) {
            size = fread(buffer, 1, BLOCK_SIZE, file);

            printf("0x%08x: ", m);

            for (n=0; n < size; n++) {
                if ((n % 8) == 0) {
                    m += 8;
                    printf("\n");
                    printf("0x%08x: ", m);
                }
                
                printf("0x%02x ", ((dword) buffer[n]) & 0xFF);
            }
        }

        fclose(file);
    }
    
    return EXIT_SUCCESS;
}

void bincat_display_usage()
{
    puts("\nAufruf: bincat <Dateiname>");
}

