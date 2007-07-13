#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "dir.h"
#include "unistd.h"
#include "config.h"

void ls_display_usage();

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_ls(int argc, char* argv[], const char* args)
#else
    #define _USE_START_
    #include "init.h"
    int main(int argc, char* argv[])
#endif
{
    if (argc > 2) {
        ls_display_usage();
        return -1;
    }
    
    char* dir_path;
    bool free_dir_path;
    bool success = TRUE;

    if (argc == 1) {
        //Mit NULL als Parameter wird der buffer mit malloc() erstellt
        dir_path = getcwd(NULL, 0);
        free_dir_path = TRUE;
    } else {
        dir_path = argv[1];
        free_dir_path = FALSE;
    }


    io_resource_t* dir_res = directory_open(dir_path);
    if (dir_res != NULL) {
        io_direntry_t* direntry;
        while ((direntry = directory_read(dir_res))) {
            if (direntry->type == IO_DIRENTRY_FILE) {
                printf("       %s\n", direntry->name);
            } else {
                printf(" [DIR] %s\n", direntry->name);
            }
           free(direntry);
        }

        directory_close(dir_res);
    } else {
        printf("Konnte '%s' nicht zum lesen oeffnen!\n", dir_path);
        success = FALSE;
    }

    if (free_dir_path == TRUE) {
        free(dir_path);
    }
    
    if (success == TRUE) {
        return EXIT_SUCCESS;
    } else {
        return -1;
    }
}

void ls_display_usage()
{
    puts("\nAufruf: ls [Verzeichnis]\n");
}

