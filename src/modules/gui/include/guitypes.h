
#ifndef GUITYPES_H
#define GUITYPES_H

#include "types.h"

typedef struct position_t
{
    int x;
    int y;
} position_t;

typedef struct dimension_t
{
    int width;
    int height;
} dimension_t;

typedef struct widget_data_t
{
    char *class;
    char *label;
    position_t position;
    dimension_t size;
    void *parent;
    void **children;
    int children_count;
    int owner;
    char *data;
} widget_data_t;

typedef widget_data_t *widget_t;

typedef struct event_t
{
    widget_t sender;
    int type;
    int data[4];
} event_t;

#endif
