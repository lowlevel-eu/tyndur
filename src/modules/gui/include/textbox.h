
#ifndef TEXTBOX_H
#define TEXTBOX_H

#include "widgetclass.h"

#include "bitmap.h"

extern widget_class_t textbox_class;
void textbox_create(widget_t textbox);
void textbox_draw(widget_t textbox);
void textbox_inject_event(widget_t textbox, event_t event);
void textbox_set_label(widget_t textbox, char *label);

typedef struct textbox_data_t
{
    char *text;
    int lines;
    bitmap_t *bitmaps;
    int *handles;
} textbox_data_t;

#endif
