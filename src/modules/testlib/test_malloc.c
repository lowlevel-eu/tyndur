#include "string.h"
#include "stdio.h"
#include "test.h"
#include "types.h"
#include "stdlib.h"


void test_malloc()
{
    void* ptr;

    printf("malloc  []: ");
    
    ptr = malloc(0);
    check(ptr == NULL);

    ptr = malloc(16);
    check(ptr != NULL);
    free(ptr);

    printf(" ");
    
    ptr = realloc(NULL, 0);
    check(ptr == NULL);

    ptr = realloc(NULL, 16);
    check(ptr != NULL);

    ptr = realloc(ptr, 0);
    check(ptr == NULL);

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

    
