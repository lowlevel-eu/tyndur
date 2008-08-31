#include "string.h"
#include "stdio.h"
#include "test.h"
#include "collections.h"

void test_list(void)
{
    list_t* list = NULL;

    printf("list    [1]: ");

    list = list_create();
    check(list != NULL);
    check(list_is_empty(list));
    check(list_get_element_at(list, 0) == NULL);
    check(list_get_element_at(list, 8) == NULL);

    printf(" ");

    check(list_push(list, (void*) 0x1234) == list);
    check(!list_is_empty(list));
    check(list_get_element_at(list, 0) == (void*) 0x1234);
    check(list_get_element_at(list, 1) == NULL);
    
    printf(" ");

    check(list_insert(list, 1, (void*) 0x2345) == list);
    check(!list_is_empty(list));
    check(list_get_element_at(list, 0) == (void*) 0x1234);
    check(list_get_element_at(list, 1) == (void*) 0x2345);
    check(list_get_element_at(list, 0) == (void*) 0x1234); // Erneut, um Caching zu testen
    check(list_get_element_at(list, 2) == NULL);
    
    printf(" ");

    check(list_push(list, (void*) 0x3456) == list);
    check(!list_is_empty(list));
    check(list_get_element_at(list, 1) == (void*) 0x1234);
    check(list_get_element_at(list, 2) == (void*) 0x2345);
    check(list_get_element_at(list, 0) == (void*) 0x3456);
    check(list_get_element_at(list, 3) == NULL);
    
    printf(" ");

    check(list_insert(list, 1, (void*) 0x4567) == list);
    check(!list_is_empty(list));
    check(list_get_element_at(list, 2) == (void*) 0x1234);
    check(list_get_element_at(list, 1) == (void*) 0x4567);
    check(list_get_element_at(list, 3) == (void*) 0x2345);
    check(list_get_element_at(list, 0) == (void*) 0x3456);
    check(list_get_element_at(list, 4) == NULL);
    
    printf(" ");
    
    check(list_insert(list, 0, (void*) 0x5678) == list);
    check(!list_is_empty(list));
    check(list_get_element_at(list, 2) == (void*) 0x4567);
    check(list_get_element_at(list, 4) == (void*) 0x2345);
    check(list_get_element_at(list, 0) == (void*) 0x5678);
    check(list_get_element_at(list, 1) == (void*) 0x3456);
    check(list_get_element_at(list, 3) == (void*) 0x1234);
    check(list_get_element_at(list, 5) == NULL);
    
    printf(" ");
    
    check(list_insert(list, 42, (void*) 0x2345) == NULL);
    check(list_push(NULL, (void*) 0x2345) == NULL);
    check(list_insert(list, -1, (void*) 0x2345) == NULL);
    check(list_remove(list, -1) == NULL);
    check(list_remove(list, 42) == NULL);
    check(list_remove(list, 5) == NULL);


    printf("\n");
    
    printf("list    [2]: ");
    
    check(list_remove(list, 2) == (void*) 0x4567);
    check(!list_is_empty(list));
    check(list_get_element_at(list, 3) == (void*) 0x2345);
    check(list_get_element_at(list, 0) == (void*) 0x5678);
    check(list_get_element_at(list, 1) == (void*) 0x3456);
    check(list_get_element_at(list, 2) == (void*) 0x1234);
    check(list_get_element_at(list, 4) == NULL);
    
    printf(" ");
    
    check(list_pop(list) == (void*) 0x5678);
    check(!list_is_empty(list));
    check(list_get_element_at(list, 1) == (void*) 0x1234);
    check(list_get_element_at(list, 2) == (void*) 0x2345);
    check(list_get_element_at(list, 0) == (void*) 0x3456);
    check(list_get_element_at(list, 3) == NULL);
    
    printf(" ");
    
    check(list_remove(list, 0) == (void*) 0x3456);
    check(!list_is_empty(list));
    check(list_get_element_at(list, 0) == (void*) 0x1234);
    check(list_get_element_at(list, 1) == (void*) 0x2345);
    check(list_get_element_at(list, 2) == NULL);
    
    printf(" ");

    check(list_remove(list, 1) == (void*) 0x2345);
    check(!list_is_empty(list));
    check(list_get_element_at(list, 0) == (void*) 0x1234);
    check(list_get_element_at(list, 1) == NULL);
    
    printf(" ");
    
    check(list_pop(list) == (void*) 0x1234);
    check(list_is_empty(list));
    check(list_get_element_at(list, 0) == NULL);
    check(list_get_element_at(list, 1) == NULL);
    
    printf("\n");
}

    
