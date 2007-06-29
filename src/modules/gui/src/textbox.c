
#include "textbox.h"
#include "widget.h"
#include "video/video.h"

#include "stdlib.h"
#include "string.h"
#include "stdio.h"

widget_class_t textbox_class =
{
    "textbox",
    textbox_create,
    textbox_draw,
    textbox_inject_event,
    textbox_set_label
};

void textbox_create(widget_t textbox)
{
    textbox->data = malloc(sizeof(textbox_data_t));
    ((textbox_data_t*)textbox->data)->text = 0;
    ((textbox_data_t*)textbox->data)->bitmap.data = 0;
    ((textbox_data_t*)textbox->data)->handle = 0;
}
void textbox_draw(widget_t textbox)
{
    widget_t parent = (widget_t)textbox->parent;
    textbox_data_t *data = (textbox_data_t*)textbox->data;
    if (data->text && data->handle) {
        int x = parent->position.x + textbox->position.x;
        int y = parent->position.y + textbox->position.y;
        video_draw_bitmap(data->handle, x, y, 0);
    }
}
void textbox_inject_event(widget_t textbox, event_t event)
{
}
void textbox_set_label(widget_t textbox, char *label)
{
    int i;
    textbox_data_t *data = (textbox_data_t*)textbox->data;
    //Aufräumen
    if (data->text) {
        free(data->text);
        data->text = 0;
    }
    if (data->bitmap.data) {
        free(data->bitmap.data);
        data->bitmap.data = 0;
    }
    if (data->handle) {
         video_delete_bitmap(data->handle);
    }
    data->handle = 0;
    
    if (label) {
        data->text = strdup(label);
        //Zeilen in Bitmaps rendern
        char *tmp = data->text;
        data->bitmap = render_string(default_font, tmp);
        if (!data->bitmap.data) {
            printf("Kein Speicher mehr?\n");
            free(data->text);
            data->text = 0;
            return;
        }
        data->handle = video_load_bitmap(data->bitmap.width,
            data->bitmap.height, data->bitmap.data);
    }
    //printf("[ GUI ] Textbox gefuellt.\n");
}
