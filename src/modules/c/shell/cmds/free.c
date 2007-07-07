#include "types.h"
#include "syscall.h"
#include "stdio.h"
#include "config.h"

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_free(int argc, char* argv[], const char* args)
#else
    #define _USE_START_
    #include "init.h"
    int main(int argc, char* argv[])
#endif
{
    memory_info_t meminfo = memory_info();

    printf("\n%10d Bytes belegt\n", meminfo.total - meminfo.free);
    printf("%10d Bytes frei\n", meminfo.free);
    printf("----------\n");
    printf("%10d Bytes gesamt\n\n", meminfo.total);

    return 0;
}
