
#ifndef WIDGET_CLASS_H
#define WIDGET_CLASS_H

#include "guitypes.h"

typedef struct widget_class_t
{
    char *name;
    void (*create)(widget_t);
    void (*draw)(widget_t);
    void (*inject_event)(widget_t, event_t);
    void (*set_label)(widget_t, char *);
} widget_class_t;

#endif
