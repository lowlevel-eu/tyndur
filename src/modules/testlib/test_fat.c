#include <stdint.h>
#include <syscall.h>
#include "string.h"
#include "stdio.h"
#include "test.h"
#include "stdlib.h"
#include "sleep.h"

unsigned long long divmod(unsigned long long dividend, unsigned int divisor, unsigned int * remainder);

void test_fat(void)
{
    FILE* f;
    void* buffer = mem_allocate(64 * 1024, 0);
    uint32_t i;

    uint64_t tickcount;
    msleep(20000);

    printf("Teste FAT-Zugriff:\n");
    for(i = 0; i < 3; i++)
    {
        tickcount = get_tick_count();
        if (!(f = fopen("floppy:/devices/fd0|fat:/modules/constest.mod.gz", "r"))) {
            printf("Konnte Datei nicht oeffnen\n");
            return;
        }

        fread(buffer, 1024, 50, f);
        fclose(f);

        printf("Das Lesen dauerte %lld ms\n", divmod((get_tick_count() - tickcount), 1000, NULL));

        msleep(10000);
    }
}

    
