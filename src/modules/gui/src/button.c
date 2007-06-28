
#include "button.h"

#include "video/video.h"
#include "bitmap.h"
#include "widget.h"

#include "stdlib.h"

widget_class_t button_class =
{
    "button",
    button_create,
    button_draw,
    button_inject_event,
    button_set_label
};

void button_create(widget_t button)
{
    button->data = malloc(sizeof(button_data_t));
    bitmap_t text = render_string(default_font, " ");
    ((button_data_t*)button->data)->textbitmap = text;
    ((button_data_t*)button->data)->label = strdup(" ");
    ((button_data_t*)button->data)->text = video_load_bitmap(text.width, text.height, text.data);
    ((button_data_t*)button->data)->state = 0;
}
void button_draw(widget_t button)
{
    bitmap_t text = ((button_data_t*)button->data)->textbitmap;
    if (button->parent){
        int x = ((widget_t)button->parent)->position.x + button->position.x;
        int y = ((widget_t)button->parent)->position.y + button->position.y;
        video_draw_rect(x, y, button->size.width, 
            button->size.height, 0xFF000000);
        video_draw_rect(x + 1, y + 1, button->size.width - 2,
            button->size.height - 2, 0xFFFFFFFF);
        if (((button_data_t*)button->data)->state == 2) {
            video_draw_rect(x + 2, y + 2, 1,
                button->size.height - 4, 0xFF000000);
            video_draw_rect(x + ((widget_data_t*)button)->size.width - 3, y + 2, 1,
                button->size.height - 4, 0xFF000000);
        }
        if (((button_data_t*)button->data)->text) {
            video_draw_bitmap(((button_data_t*)button->data)->text, 
                x + ((widget_data_t*)button)->size.width / 2 - text.width / 2,
                y + button->size.height / 2 - text.height / 2, 0);
        }
    } else {
        video_draw_rect(button->position.x, button->position.y,
            button->size.width, button->size.height, 0xFF000000);
        video_draw_rect(button->position.x + 1, button->position.y + 1,
            button->size.width - 2, button->size.height - 2, 0xFFFFFFFF);
        if (((button_data_t*)button->data)->text) {
            video_draw_bitmap(((button_data_t*)button->data)->text,
                button->position.x + button->size.width / 2 - text.width / 2,
                button->position.y + button->size.height / 2 - text.height / 2,
                0);
        }
    }
}
void button_inject_event(widget_t button, event_t event)
{
    //printf("Button:\n");
    switch (event.type)
    {
        case EVENT_MOUSE_ENTERED_AREA:
            ((button_data_t*)button->data)->state = 1;
            break;
        case EVENT_MOUSE_LEFT_AREA:
            ((button_data_t*)button->data)->state = 0;
            break;
        case EVENT_MOUSE_BUTTON_PRESSED:
            ((button_data_t*)button->data)->state = 2;
            break;
        case EVENT_MOUSE_BUTTON_RELEASED:
            if (((button_data_t*)button->data)->state == 2) {
                ((button_data_t*)button->data)->state = 1;
                event_t newevent;
                newevent.sender = button;
                newevent.type = EVENT_BUTTON_CLICKED;
                send_gui_event(button->owner, newevent);
            }
            break;
    }
    //printf("state = %d\n", ((button_data_t*)button->data)->state);
}

void button_set_label(widget_t button, char *label)
{
    video_delete_bitmap(((button_data_t*)button->data)->text);
    free(((button_data_t*)button->data)->textbitmap.data);
    free(((button_data_t*)button->data)->label);
    ((button_data_t*)button->data)->label = strdup(label);
    bitmap_t text = render_string(default_font, label);
    ((button_data_t*)button->data)->textbitmap = text;
    ((button_data_t*)button->data)->text = video_load_bitmap(text.width, text.height, text.data);
}
