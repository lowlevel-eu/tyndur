#include "string.h"
#include "stdio.h"
#include "test.h"

void test_strlen(void)
{
    char * empty = "";
    char * one = "1";
    char * hello = "Hello, world!";

    check(strlen(empty) == 0);
    check(strlen(one) == 1);
    check(strlen(hello) == 13);

    printf(" ");
    check(strnlen(empty, 0) == 0);
    check(strnlen(one, 0) == 0);
    check(strnlen(one, 1) == 1);
    check(strnlen(one, 5) == 1);
    check(strnlen(hello, 12) == 12);
    check(strnlen(hello, 13) == 13);
    check(strnlen(hello, 14) == 13);
}

void test_strcmp(void)
{
    char * empty = "";
    char * a = "Aepfel";
    char * b = "Birnen";
    char * c = "Birnen";
    char * d = "Bananen";
    char * e = "Bananenschale";

    check(strcmp(empty, empty) == 0);
    check(strcmp(a, empty) > 0);
    check(strcmp(empty, a) < 0);
    
    check(strcmp(a, b) < 0);
    check(strcmp(b, a) > 0);
    
    check(strcmp(a, a) == 0);
    check(strcmp(b, c) == 0);
    
    check(strcmp(c, d) > 0);
    
    check(strcmp(d, e) < 0);
    check(strcmp(e, d) > 0);
    
    printf(" ");
    check(strncmp(empty, a, 0) == 0);
    check(strncmp(a, empty, 2) > 0);
    check(strncmp(a, b, 0) == 0);
    
    printf(" ");
    check(strncmp(a, b, 4) < 0);
    check(strncmp(b, c, 4) == 0);
    check(strncmp(d, e, 7) == 0);
    check(strncmp(d, e, 8) < 0);
}

void test_string(void)
{
    printf("string  [1]: ");
    test_strlen();
    printf("  ");
    test_strcmp();
    printf("\n");
}

    
