#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include "types.h"

typedef struct {
    struct list_node* anchor;
    dword size;
} list_t;

list_t* list_create();
void list_destroy(list_t* list);
list_t* list_push(list_t* list, void* value);
void* list_pop(list_t* list);
bool list_is_empty(list_t* list);
void* list_get_element_at(list_t* list, int index);
list_t* list_insert(list_t* list, int index, void* value);
void* list_remove(list_t* list, int index);
dword list_size(list_t* list);

#endif
