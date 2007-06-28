
#ifndef BUTTON_H
#define BUTTON_H

#include "widgetclass.h"
#include "bitmap.h"

extern widget_class_t button_class;

void button_create(widget_t button);
void button_draw(widget_t button);
void button_inject_event(widget_t button, event_t event);
void button_set_label(widget_t button, char *label);

typedef struct button_data_t
{
    int text;
    char *label;
    bitmap_t textbitmap;
    int state;
} button_data_t;

#endif
