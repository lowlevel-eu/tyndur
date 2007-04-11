#include "string.h"
#include "stdio.h"
#include "test.h"
#include "types.h"
#include "stdlib.h"

void test_fat()
{
    FILE* f;
    void* buffer = mem_allocate(64 * 1024, 0);
    qword tickcount;
    msleep(20000);
    while(TRUE)
    {
        tickcount = get_tick_count();
        f = fopen("floppy:/devices/fd0|fat:/modules/constest.mod.gz", "r");
        fread(buffer, 1024, 50, f);
        fclose(f);

        printf("Das Lesen dauerte %d ms\n", divmod((get_tick_count() - tickcount), 1000, NULL));

        msleep(10000);
    }
}

    
