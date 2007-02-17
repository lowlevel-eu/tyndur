#include "collections.h"
#include "types.h"
#include "stdlib.h"

struct list_node {
    struct list_node* next;
    void* value;
};

list_t* list_create() 
{
    list_t* list = malloc(sizeof(list_t));
    list->anchor = NULL;

    return list;
}

list_t* list_push(list_t* list, void* value)
{
    if (!list) {
        return NULL;
    }

    struct list_node* element = malloc(sizeof(struct list_node));
    element->value = value;
    element->next = list->anchor;

    list->anchor = element;
    return list;
}

void* list_pop(list_t* list)
{
    struct list_node* old_anchor;
    void* value;
    
    if (!list) {
        return NULL;
    }
    
    if (list->anchor) {
        value = list->anchor->value;

        old_anchor = list->anchor;
        list->anchor = list->anchor->next;
        free(old_anchor);
    } else {
        value = NULL;
    }

    return value;
}
    
bool list_is_empty(list_t* list)
{
    return (!list) || (list->anchor == NULL);
}

void* list_get_element_at(list_t* list, int index)
{
    static list_t* last_list = NULL;
    static int last_index = 0;
    static struct list_node* last_node = NULL;
    
    if (!list || !list->anchor) {
        return NULL;
    }

    struct list_node* current = list->anchor;
    if ((last_list == list) && (last_index <= index) && (last_node != NULL)) {
        index -= last_index;
        current = last_node;
    }
    
    int n = index;
    while (n--) {
        current = current->next;
        if (!current) {
            return NULL;
        }
    }

    last_list = list;
    last_index = index;
    last_node = current;

    return current->value;
}
