
#include "panel.h"

#include "video/video.h"

widget_class_t panel_class =
{
    "panel",
    panel_create,
    panel_draw,
    panel_inject_event,
    0
};

void panel_create(widget_t panel)
{
}

void panel_draw(widget_t panel)
{
    /*if (panel->parent){
        video_draw_rect(((widget_t)panel->parent)->position.x + panel->position.x,
                          ((widget_t)panel->parent)->position.y + panel->position.y, panel->size.width,
                          panel->size.height, 0xFF0000FF);
    } else {
        video_draw_rect(panel->position.x, panel->position.y,
                        panel->size.width, panel->size.height, 0xFF0000FF);
    }*/
}
void panel_inject_event(widget_t panel, event_t event)
{
    //printf("Panel!\n");
}