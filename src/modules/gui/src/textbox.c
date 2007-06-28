
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
    ((textbox_data_t*)textbox->data)->lines = 0;
    ((textbox_data_t*)textbox->data)->bitmaps = 0;
    ((textbox_data_t*)textbox->data)->handles = 0;
}
void textbox_draw(widget_t textbox)
{
    int i;
    textbox_data_t *data = (textbox_data_t*)textbox->data;
    if (((textbox_data_t*)textbox->data)->text) {
        widget_t parent = (widget_t)textbox->parent;
        int x = parent->position.x + textbox->position.x;
        int y = parent->position.y + textbox->position.y;
        for (i = 0; i < data->lines; i++) {
            if (data->handles[i]) {
                video_draw_bitmap(data->handles[i], x, y + i * 15, 0);
                //printf("Zeichne Zeile %d: %d\n", i, data->bitmaps[i].width);
            }
        }
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
    data->text = 0;
    int lines = data->lines;
    data->lines = 0;
    if (data->bitmaps) {
        for (i = 0; i < lines; i++) {
            if (data->bitmaps[i].data) {
                free(data->bitmaps[i].data);
                data->bitmaps[i].data = 0;
            }
        }
        free(data->bitmaps);
        data->bitmaps = 0;
    }
     if (data->handles) {
        for (i = 0; i < lines; i++) {
            video_delete_bitmap(data->handles[i]);
            data->handles[i] = 0;
        }
        free(data->handles);
        data->handles = 0;
    }
    //Text aufsplitten
    data->text = strdup(label);
    data->lines = 1;
    for (i = 0; i < strlen(label); i++) {
        if (data->text[i] == '\n') {
            data->lines++;
            data->text[i] = 0;
        }
    }
    //Zeilen in Bitmaps rendern
    data->bitmaps = malloc(sizeof(bitmap_t) * data->lines);
    data->handles = malloc(sizeof(int) * data->lines);
    char *tmp = data->text;
    for (i = 0; i < data->lines; i++) {
        //printf("Fuege Zeile hinzu: \"%s\"\n", tmp);
        data->bitmaps[i] = render_string(default_font, tmp);
        data->handles[i] = video_load_bitmap(data->bitmaps[i].width,
            data->bitmaps[i].height, data->bitmaps[i].data);
        tmp += strlen(tmp) + 1;
    }
    printf("[ GUI ] Textbox gefuellt, %d Zeilen!\n", data->lines);
}
