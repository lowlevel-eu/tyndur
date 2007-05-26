#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "dir.h"
#include "config.h"

void mkdir_display_usage();

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_mkdir(int argc, char* argv[], const char* args)
#else
    #define _USE_START_
    #include "init.h"
    int main(int argc, char* argv[])
#endif
{
    if (argc != 2) {
        mkdir_display_usage();
        return -1;
    }

    if (directory_create(argv[1]) == FALSE) {
        printf("mkdir: kann Verzeichnis '%s' nicht anlegen\n", argv[1]);
    }


    return EXIT_SUCCESS;
}

void mkdir_display_usage()
{
    puts("\nAufruf: mkdir <Verzeichnisname>");
}

