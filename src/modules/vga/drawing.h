
#ifndef DRAWING_H
#define DRAWING_H

void clear_screen(int color);
void draw_dot(int x, int y, int color);
void draw_rect(int x, int y, int width, int height, int color);
void draw_bitmap(int bitmap, int x, int y, int flags);

int create_bitmap(int width, int height, int *data);
int delete_bitmap(int bitmap);

int update_screen(void);
int update_partly(int x, int y, int width, int height);

#endif
