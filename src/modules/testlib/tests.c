#include "test.h"
#include "stdio.h"

extern int test_string(void);
extern int test_list(void);
extern int test_avl(void);
extern void test_malloc(void);
extern void test_printf(void);
extern void test_fat(void);

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
    test_printf();
//    test_fat();
    test_avl();

    return 0;
}
