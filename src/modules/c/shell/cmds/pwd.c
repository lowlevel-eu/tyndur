#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "config.h"

void pwd_display_usage();

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_pwd(int argc, char* argv[], const char* args)
#else
    #define _USE_START_
    #include "init.h"
    int main(int argc, char* argv[])
#endif
{
    if (argc > 1) {
        pwd_display_usage();
        return -1;
    }

    char* cwd = getcwd(NULL, 0); 
    puts(cwd);
    free(cwd);

    return EXIT_SUCCESS;
}

void pwd_display_usage()
{
    puts("\nAufruf: pwd");
}

