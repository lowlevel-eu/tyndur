#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include "types.h"

typedef struct {
    struct list_node* anchor;
} list_t;

list_t* list_create();
list_t* list_push(list_t* list, void* value);
void* list_pop(list_t* list);
bool list_is_empty(list_t* list);
void* list_get_element_at(list_t* list, int index);

#endif
