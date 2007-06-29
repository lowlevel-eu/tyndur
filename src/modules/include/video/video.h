
#ifndef VIDEO_H
#define VIDEO_H

int video_init(char *driver_name);
void video_close(void);

int video_start_command_chain(void);
int video_send_command_chain(void);

int video_draw_dot(int x, int y, int color);
int video_draw_rect(int x, int y, int width, int height, int color);
int video_clear_screen(int color);

int video_draw_bitmap(int bitmap, int x, int y, int transparent);

int video_load_bitmap(int width, int height, void *data);
int video_delete_bitmap(int bitmap);

int video_update_screen(void);
int video_update_partly(int x, int y, int width, int height);

#endif
