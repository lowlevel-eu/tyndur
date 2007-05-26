#include "types.h"
#include "stdio.h"
#include "stdlib.h"
#include "config.h"

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_echo(int argc, char* argv[], const char* args)
#else
    #define _USE_START_
    #include "init.h"
    int main(int argc, char* argv[])
#endif
{
    int i;

    for (i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    putchar('\n');

    return EXIT_SUCCESS;
}

