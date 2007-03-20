#include "string.h"
#include "stdio.h"
#include "test.h"
#include "types.h"
#include "stdlib.h"

#define MEM_BLOCK_COUNT 10

dword block_sizes[MEM_BLOCK_COUNT] = { 
    16, 32, 42, 1, 8, 
    753, 4095, 4096, 4097, 68332
};

void* blocks[MEM_BLOCK_COUNT] = { NULL };

bool mem_overlaps(void* ptr, dword size)
{
    dword i;
    for (i = 0; i < MEM_BLOCK_COUNT; i++) {
        if (blocks[i] != NULL) {
            if (ptr < blocks[i]) {
                if (ptr + size > blocks[i]) {
                    return TRUE;
                }
            } else {
                if (blocks[i] + block_sizes[i] > ptr) {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

void test_malloc()
{
    void* ptr;
    dword i;

    printf("malloc  [1]: ");
    
    /*
    // Implementation defined, daher deaktiviert
    ptr = malloc(0);
    check(ptr == NULL);
    */

    ptr = malloc(16);
    check(ptr != NULL);
    free(ptr);

    printf(" ");
    
    for (i = 0; i < MEM_BLOCK_COUNT; i++) {
        ptr = malloc(block_sizes[i]);
        check(!mem_overlaps(ptr, block_sizes[i]));
        blocks[i] = ptr;
    }
    
    printf(" ");

    free(blocks[7]);
    blocks[7] = NULL;
    block_sizes[7] = 73;
    ptr = malloc(block_sizes[7]);
    check(!mem_overlaps(ptr, block_sizes[7]));
    blocks[7] = ptr;
    
    free(blocks[8]);
    blocks[8] = NULL;
    block_sizes[8] = 7323;
    ptr = malloc(block_sizes[8]);
    check(!mem_overlaps(ptr, block_sizes[8]));
    blocks[8] = ptr;

    ptr = malloc(111);
    check(!mem_overlaps(ptr, 111));

    printf(" ");
    
    /*
    // Implementation defined, daher deaktiviert
    ptr = realloc(NULL, 0);
    check(ptr == NULL);
    */

    ptr = realloc(NULL, 16);
    check(ptr != NULL);

    /*
    // Implementation defined, daher deaktiviert
    ptr = realloc(ptr, 0);
    check(ptr == NULL);
    */

    qword* qptr;
    qptr = malloc(4096 * 2);
    *qptr = 0x1234567;
    *((qword*) ((dword) qptr + 512 - sizeof(qword))) = 0x7654321;
    qptr = realloc(qptr, 512);
    check(*qptr == 0x1234567);
    check(*((qword*) ((dword) qptr + 512 - sizeof(qword))) == 0x7654321);
    free(qptr);

    printf("\n");
}

    
