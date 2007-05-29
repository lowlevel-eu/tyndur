#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "io.h"
#include "config.h"

void symlink_display_usage();

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_symlink(int argc, char* argv[], const char* args)
#else
    #define _USE_START_
    #include "init.h"
    int main(int argc, char* argv[])
#endif
{
    if (argc != 3) {
        symlink_display_usage();
        return -1;
    }

    FILE* file = fopen(argv[1], "lw");
    
    if (file == NULL) {
        puts("Fehler: Symlink konnte nicht erstellt werden.");
        return -1;
    }
    
    char* dest = io_get_absolute_path(argv[2]);
    fwrite(dest, 1, strlen(dest) + 1, file);
    fclose(file);

    return EXIT_SUCCESS;
}

void symlink_display_usage()
{
    puts("\nAufruf: symlink <Verknuepfungsname> <Pfad>");
}

