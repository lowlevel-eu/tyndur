
#include "widget.h"

#include "rpc.h"

#include "stdio.h"
#include "stdlib.h"

widget_class_t *globalclasses;
dword globalclasses_count;

widget_t rootwidget = 0;

font_t default_font;

extern int screen_width;
extern int screen_height;

extern int mousex;
extern int mousey;

int dragging = 0;
widget_t draggingobject;

int init_root_widget(void)
{
    rootwidget = malloc(sizeof(widget_data_t));
    rootwidget->class = "panel";
    rootwidget->parent = 0;
    rootwidget->position.x = 0;
    rootwidget->position.y = 0;
    rootwidget->size.width = screen_width;
    rootwidget->size.height = screen_height;
    rootwidget->children = 0;
    rootwidget->children_count = 0;
    rootwidget->owner = -1;
    return 1;
}

void set_default_font(font_t font)
{
    default_font = font;
}

int register_widget_class(int owner, widget_class_t class)
{
    if (owner == -1) {
        widget_class_t *tmp_globalclasses = malloc(sizeof(widget_class_t) * (globalclasses_count + 1));
        memcpy(tmp_globalclasses, globalclasses, globalclasses_count * sizeof(widget_class_t));
        tmp_globalclasses[globalclasses_count] = class;
        if (globalclasses_count > 0) {
            free(globalclasses);
        }
        globalclasses = tmp_globalclasses;
        globalclasses_count++;
        return 1;
    } else {
        printf("[ GUI ] Eigene Widgets noch nicht implementiert.\n");
        return 0;
    }
}
int delete_widget_class(char *name)
{
    return 0;
}
widget_class_t *get_widget_class(char *name)
{
    int i;
    for (i = 0; i < globalclasses_count; i++)
    {
        if (!strcmp(globalclasses[i].name, name)) {
            return &globalclasses[i];
        }
    }
    return 0;
}

widget_t create_widget(char *widgetclass, widget_t parent, int process)
{
    if (!get_widget_class(widgetclass)) {
        printf("[ GUI ] Klasse \"%s\" nicht gefunden!\n", widgetclass);
        return 0;
    }
    widget_t newwidget = malloc(sizeof(widget_data_t));
    newwidget->class = widgetclass;
    if (parent) {
        newwidget->parent = parent;
    } else {
        newwidget->parent = rootwidget;
    }
    add_child(newwidget->parent, newwidget);
    newwidget->position.x = 0;
    newwidget->position.y = 0;
    newwidget->size.width = 0;
    newwidget->size.height = 0;
    newwidget->children = 0;
    newwidget->children_count = 0;
    newwidget->owner = process;
    get_widget_class(newwidget->class)->create(newwidget);
    return newwidget;
}
widget_t create_widget_ex(char *widgetclass, widget_t parent, position_t position, dimension_t size, int process)
{
    if (!get_widget_class(widgetclass)) {
        printf("[ GUI ] Klasse \"%s\" nicht gefunden!\n", widgetclass);
        return 0;
    }
    widget_t newwidget = malloc(sizeof(widget_data_t));
    newwidget->class = widgetclass;
    if (parent) {
        newwidget->parent = parent;
    } else {
        newwidget->parent = rootwidget;
    }
    add_child(newwidget->parent, newwidget);
    newwidget->position = position;
    newwidget->size = size;
    newwidget->children = 0;
    newwidget->children_count = 0;
    newwidget->owner = process;
    get_widget_class(newwidget->class)->create(newwidget);
    return newwidget;
}
void delete_widget(widget_t widget)
{
    if (widget->parent) {
        delete_child(widget->parent, widget);
    }
    while (widget->children_count > 0) {
        delete_widget((widget_t)widget->children[0]);
    }
    free(widget);
}

void add_child(widget_t parent, widget_t child)
{
    void **tmp_children = malloc(sizeof(void *) * (parent->children_count + 1));
    memcpy(tmp_children, parent->children, parent->children_count * sizeof(void *));
    tmp_children[parent->children_count] = child;
    if (parent->children_count > 0) {
        free(parent->children);
    }
    parent->children = tmp_children;
    parent->children_count++;
}
void delete_child(widget_t parent, widget_t child)
{
    int i;
    int position = -1;
    for (i = 0; i < parent->children_count; i++) {
        if (parent->children[i] == child) {
            position = i;
            break;
        }
    }
    if (position == -1) return;
    
    if (parent->children_count == 1) {
        free(parent->children);
        parent->children = 0;
        parent->children_count = 0;
    } else {
        void **tmp_children = malloc(sizeof(void *) * (parent->children_count - 1));
        memcpy(tmp_children, parent->children, position * sizeof(void *));
        memcpy(tmp_children + position, parent->children + position + 1,
               (parent->children_count - position - 1) * sizeof(void *));
        free(parent->children);
        parent->children = tmp_children;
        parent->children_count--;
    }
}
void move_to_front(widget_t widget)
{
    if (widget->parent) {
        int i;
        int position = -1;
        widget_t parent = widget->parent;
        for (i = 0; i < parent->children_count; i++) {
            if (parent->children[i] == widget) {
                position = i;
                break;
            }
        }
        if (position == -1) return;
        
        for (i = position; i < parent->children_count - 1; i++) {
            parent->children[i] = parent->children[i] + 1;
        }
        parent->children[parent->children_count - 1] = widget;
        move_to_front(parent);
    }
}

void start_dragging(widget_t widget)
{
    dragging = 1;
    draggingobject = widget;
}

void set_position(widget_t widget, position_t position)
{
    widget->position.x = position.x;
    widget->position.y = position.y;
}
void set_size(widget_t widget, dimension_t size)
{
    widget->size.width = size.width;
    widget->size.height = size.height;
}
void set_label(widget_t widget, char *label)
{
    if (get_widget_class(widget->class)->set_label) {
        get_widget_class(widget->class)->set_label(widget, label);
    }
}

int draw_widget(widget_t widget)
{
    get_widget_class(widget->class)->draw(widget);
    return 1;
}
int draw_widget_recursive(widget_t widget)
{
    get_widget_class(widget->class)->draw(widget);
    int i;
    for (i = 0; i < widget->children_count; i++) {
        draw_widget_recursive(widget->children[i]);
    }
    return 1;
}
int draw_all(void)
{
    draw_widget_recursive(rootwidget);
    return 1;
}

widget_t get_widget_at_position(widget_t parent, int x, int y)
{
    int i;
    if (!parent) parent = rootwidget;
    if (!parent) return 0;
    if ((x < parent->position.x) || (y < parent->position.y)
         || (x > parent->position.x + parent->size.width - 1)
         || (y > parent->position.y + parent->size.height - 1)) {
        return 0;
    }
    x -= parent->position.x;
    y -= parent->position.y;
    widget_t *children = (widget_t*)parent->children;
    for (i = parent->children_count - 1; i >= 0; i--) {
        if ((x >= children[i]->position.x) && (x < children[i]->position.x + children[i]->size.width)
             && (y >= children[i]->position.y) && (y < children[i]->position.y + children[i]->size.height)) {
            return children[i];
        }
    }
    return 0;
}

void inject_mouse_move(int x, int y)
{
    if (!dragging) {
        event_t event;
        event.sender = 0;
        event.type = EVENT_MOUSE_MOVED;
        event.data[0] = x;
        event.data[1] = y;
        event.data[2] = mousex;
        event.data[3] = mousey;
        inject_mouse_event(rootwidget, event);
    } else {
        draggingobject->position.x += x;
        draggingobject->position.y += y;
    }
}
void inject_mouse_button_pressed(int button)
{
    event_t event;
    event.sender = 0;
    event.type = EVENT_MOUSE_BUTTON_PRESSED;
    event.data[0] = button;
    inject_mouse_event(rootwidget, event);
}
void inject_mouse_button_released(int button)
{
    if (!dragging) {
        event_t event;
        event.sender = 0;
        event.type = EVENT_MOUSE_BUTTON_RELEASED;
        event.data[0] = button;
        inject_mouse_event(rootwidget, event);
    } else {
        dragging = 0;
    }
}

void inject_mouse_event(widget_t widget, event_t event)
{
    int event_consumed = 0;
    widget_t child1;
    widget_t child2;
    //An untergeordnete Widgets weitergeben
    switch (event.type)
    {
        case EVENT_MOUSE_MOVED:
            //printf("[ GUI ] Maus hat sich bewegt.\n");
            //Derzeitige Position:
            child1 = get_widget_at_position(widget, event.data[2],
                event.data[3]);
            //Alte Position:
            child2 = get_widget_at_position(widget,
                event.data[2] - event.data[0], event.data[3] - event.data[1]);
            //Weiterleiten?
            if (child1 == child2) {
                if (child1) {
                    event_t newevent = event;
                    newevent.sender = widget;
                    inject_mouse_event(child1, newevent);
                }
            } else {
                if (child1) {
                    //printf("[ GUI ] Sende EVENT_MOUSE_ENTERED_AREA.\n");
                    event_t newevent = event;
                    newevent.sender = widget;
                    newevent.type = EVENT_MOUSE_ENTERED_AREA;
                    inject_mouse_event(child1, newevent);
                }
                if (child2) {
                    //printf("[ GUI ] Sende EVENT_MOUSE_LEFT_AREA.\n");
                    event_t newevent = event;
                    newevent.sender = widget;
                    newevent.type = EVENT_MOUSE_LEFT_AREA;
                    inject_mouse_event(child2, newevent);
                }
            }
            break;
        case EVENT_MOUSE_ENTERED_AREA:
            //printf("[ GUI ] Maus hat Gebiet betreten.\n");
            //Derzeitige Position:
            child1 = get_widget_at_position(widget, event.data[2],
                event.data[3]);
            //Weiterleiten?
            if (child1) {
                event_t newevent = event;
                newevent.sender = widget;
                newevent.type = EVENT_MOUSE_ENTERED_AREA;
                inject_mouse_event(child1, newevent);
            }
            break;
        case EVENT_MOUSE_LEFT_AREA:
            //printf("[ GUI ] Maus hat Gebiet verlassen.\n");
            //Alte Position:
            child1 = get_widget_at_position(widget,
                event.data[2] - event.data[0], event.data[3] - event.data[1]);
            //Weiterleiten?
            if (child1) {
                event_t newevent = event;
                newevent.sender = widget;
                newevent.type = EVENT_MOUSE_LEFT_AREA;
                inject_mouse_event(child1, newevent);
            }
            break;
        case EVENT_MOUSE_BUTTON_PRESSED:
            //printf("[ GUI ] Mausbutton wurde gedrueckt (%d).\n", event.data[0]);
            child1 = get_widget_at_position(widget, mousex, mousey);
            if (child1) {
                event_t newevent = event;
                newevent.sender = widget;
                newevent.type = EVENT_MOUSE_BUTTON_PRESSED;
                inject_mouse_event(child1, newevent);
                event_consumed = 1;
            }
            break;
        case EVENT_MOUSE_BUTTON_RELEASED:
            //printf("[ GUI ] Mausbutton wurde losgelassen (%d).\n", event.data[0]);
            child1 = get_widget_at_position(widget, mousex, mousey);
            if (child1) {
                event_t newevent = event;
                newevent.sender = widget;
                newevent.type = EVENT_MOUSE_BUTTON_RELEASED;
                inject_mouse_event(child1, newevent);
                event_consumed = 1;
            }
            break;
    }
    
    if (!widget) return;
    //evtl. Event selber verarbeiten
    if (!event_consumed) {
        if (widget && widget->class) {
            widget_class_t *class = get_widget_class(widget->class);
            if (class && class->inject_event) {
                get_widget_class(widget->class)->inject_event(widget, event);
            }
        }
    }
}

int callback_count = 0;
char **callbackfunctions = 0;
int *callbackpids = 0;

int register_callback_function(int process, char *name)
{
    char **tmp_callbackfunctions = malloc(sizeof(char *) * (callback_count + 1));
    memcpy(tmp_callbackfunctions, callbackfunctions, callback_count * sizeof(char *));
    tmp_callbackfunctions[callback_count] = strdup(name);
    if (callback_count > 0) {
        free(callbackfunctions);
    }
    callbackfunctions = tmp_callbackfunctions;
    
    int *tmp_callbackpids = malloc(sizeof(int) * (callback_count + 1));
    memcpy(tmp_callbackpids, callbackpids, callback_count * sizeof(int));
    tmp_callbackpids[callback_count] = process;
    if (callback_count > 0) {
        free(callbackpids);
    }
    callbackpids = tmp_callbackpids;
    
    callback_count++;
    return 1;
}
int delete_callback_function(int process)
{
    return 0;
}
void send_gui_event(int process, event_t event)
{
    //printf("[ GUI ] GUI-Event wird an Prozess %d gesendet.\n", process);
    if (process != -1) {
        int i;
        for (i = 0; i < callback_count; i++) {
            if (callbackpids[i] == process) {
                rpc_get_dword(callbackpids[i],
                              callbackfunctions[i],
                              sizeof(event_t),
                              (char*)&event);
            }
        }
    }
}
