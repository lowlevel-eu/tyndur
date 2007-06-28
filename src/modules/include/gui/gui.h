
#ifndef GUI_GUI_H
#define GUI_GUI_H

typedef int widget_t;

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

typedef struct event_t
{
    widget_t sender;
    int type;
    int data[4];
} event_t;

#define EVENT_BUTTON_CLICKED 5

int init_gui(void);
int close_gui(void);

widget_t gui_create_widget(char *class, widget_t parent);
widget_t gui_create_widget_ex(char *class, widget_t parent, position_t position, dimension_t size);
int gui_delete_widget(widget_t widget);

int gui_set_position(widget_t widget, position_t position);
int gui_set_size(widget_t widget, dimension_t size);
int gui_set_label(widget_t widget, char *label);

void gui_set_callback(void (*cb)(event_t));

#endif
