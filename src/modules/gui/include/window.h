
#ifndef WINDOW_H
#define WINDOW_H

#include "widgetclass.h"

#include "bitmap.h"

extern widget_class_t window_class;
void window_create(widget_t window);
void window_draw(widget_t window);
void window_inject_event(widget_t window, event_t event);
void window_set_label(widget_t window, char *label);

typedef struct window_data_t
{
    int text;
    char *label;
    bitmap_t textbitmap;
    int state;
} window_data_t;

#endif
