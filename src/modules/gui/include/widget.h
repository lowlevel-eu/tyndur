
#ifndef WIDGET_H
#define WIDGET_H

#include "widgetclass.h"
#include "font.h"

extern font_t default_font;

#define EVENT_MOUSE_MOVED 0
#define EVENT_MOUSE_ENTERED_AREA 1
#define EVENT_MOUSE_LEFT_AREA 2
#define EVENT_MOUSE_BUTTON_PRESSED 3
#define EVENT_MOUSE_BUTTON_RELEASED 4

#define EVENT_BUTTON_CLICKED 5

int init_root_widget(void);

void set_default_font(font_t font);

//Widgetklassen
int register_widget_class(int owner, widget_class_t class);
int delete_widget_class(char *name);
widget_class_t *get_widget_class(char *name);

//Management
widget_t create_widget(char *widgetclass, widget_t parent, int process);
widget_t create_widget_ex(char *widgetclass, widget_t parent, position_t position, dimension_t size, int process);
void delete_widget(widget_t widget);

void add_child(widget_t parent, widget_t child);
void delete_child(widget_t parent, widget_t child);
void move_to_front(widget_t widget);

void start_dragging(widget_t widget);

//Eigenschaften
void set_position(widget_t widget, position_t position);
void set_size(widget_t widget, dimension_t size);
void set_label(widget_t widget, char *label);

//Rendern
int draw_widget(widget_t widget);
int draw_widget_recursive(widget_t widget);
int draw_all(void);

widget_t get_widget_at_position(widget_t parent, int x, int y);

//Input
void inject_mouse_move(int x, int y);
void inject_mouse_button_pressed(int button);
void inject_mouse_button_released(int button);

void inject_mouse_event(widget_t widget, event_t event);

//Callback
int register_callback_function(int process, char *name);
int delete_callback_function(int process);
void send_gui_event(int process, event_t event);

#endif
