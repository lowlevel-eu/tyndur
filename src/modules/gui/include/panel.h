
#ifndef PANEL_H
#define PANEL_H

#include "widgetclass.h"

extern widget_class_t panel_class;
void panel_create(widget_t panel);
void panel_draw(widget_t panel);
void panel_inject_event(widget_t panel, event_t event);

#endif
