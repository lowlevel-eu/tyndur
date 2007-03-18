#define _USE_START_
#include "init.h"
#include "test.h"
#include "stdio.h"

extern int test_string();
extern int test_list();
extern void test_malloc();

void passed()
{
    printf(".");
}

void failed()
{
    printf("!");
}

void check(int condition)
{
    if (condition) {
        passed();
    } else {
        failed();
    }
}

int main(int argc, char* argv[])
{
    test_string();
    test_list();
    test_malloc();

    return 0;
}
