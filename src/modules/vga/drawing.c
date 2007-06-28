
#include "stdlib.h"

#include "drawing.h"
#include "vga.h"
#include "palette.h"

int bitmap_count = 0;
byte **bitmap_data = 0;
int *bitmap_width = 0;
int *bitmap_height= 0;
int *bitmap_handle = 0;

inline void draw_dot_4p(int x, int y, byte c)
{
    //printf("draw_dot_4p 1\n");
    /*unsigned wd_in_bytes, off, mask, p, pmask;

    wd_in_bytes = screen_xres / 8;
    off = wd_in_bytes * y + x / 8;
    x = (x & 7) * 1;
    mask = 0x80 >> x;
    pmask = 1;
    for(p = 0; p < 4; p++)
    {
        //vga_set_plane(p);
        if(pmask & c)
            (backbuffer + p * screen_xres * screen_yres / 8)[off] = (backbuffer + p * screen_xres * screen_yres / 8)[off] | mask;
        else
            (backbuffer + p * screen_xres * screen_yres / 8)[off] = (backbuffer + p * screen_xres * screen_yres / 8)[off] & ~mask;
        pmask <<= 1;
    }*/
    //Optimiert:
    unsigned wd_in_bytes, off, mask, p, pmask, pagesize;
    
    wd_in_bytes = screen_xres / 8;
    pagesize = screen_yres * wd_in_bytes;
    off = wd_in_bytes * y + x / 8;
    x = (x & 7) * 1;
    mask = 0x80 >> x;
    pmask = 1;
    for(p = 0; p < 4; p++)
    {
        if(pmask & c)
            backbuffer[off] = backbuffer[off] | mask;
        else
            backbuffer[off] = backbuffer[off] & ~mask;
        pmask <<= 1;
        off += pagesize;
    }
    //printf("draw_dot_4p 2\n");
}

void clear_screen(int color)
{
    //printf("clear_screen 1\n");
    if (!backbuffer) {
        return;
    }
    byte vgacolor = vga_get_color(color);
    int pagesize;
    switch (screen_bpp)
    {
        case 8:
            //draw_rect(0, 0, screen_xres, screen_yres, color);
            memset(backbuffer, vgacolor, screen_xres * screen_yres);
            //printf("clear_screen(8) 2\n");
            return;
        case 4:
            vgacolor = vga_get_color(color);
            pagesize = screen_xres * screen_yres / 8;
            memset(backbuffer, 0xFF * ((color & 8) >> 3), pagesize);
            memset(backbuffer + pagesize, 0xFF * ((color & 4) >> 2), pagesize);
            memset(backbuffer + pagesize * 2, 0xFF * ((color & 2) >> 1), pagesize);
            memset(backbuffer + pagesize * 3, 0xFF * (color & 1), pagesize);
            //printf("clear_screen(4) 2\n");
            return;
    }
}
void draw_dot(int x, int y, int color)
{
    //printf("draw_dot 1\n");
    if (!backbuffer) {
        return;
    }
    if ((x >= 0) && (x < screen_xres) && (y >= 0) && (y < screen_yres)) {
        switch (screen_bpp)
        {
            case 8:
                backbuffer[x + y * screen_xres] = vga_get_color(color);
                //printf("draw_dot(8) 2\n");
                return;
            case 4:
                draw_dot_4p(x, y, vga_get_color(color));
                //printf("draw_dot(4) 2\n");
                return;
        }
    }
}
void draw_rect(int x, int y, int width, int height, int color)
{
    //printf("draw_rect 1\n");
    if (!backbuffer) {
        return;
    }
    byte vgacolor = vga_get_color(color);
    /*int ix;
    int iy;
    for (ix = x; ix < x + width; ix++) {
        for (iy = y; iy < y + height; iy++) {
            if ((ix >= 0) && (ix < screen_xres) && (iy >= 0) && (iy < screen_yres)) {
                switch (screen_bpp)
                {
                    case 8:
                        backbuffer[ix + iy * screen_xres] = 
                                vgacolor;
                        break;
                    case 4:
                        draw_dot_4p(ix, iy, vgacolor);
                        break;
                }
            }
        }
    }*/
    if ((x >= screen_xres) || (y >= screen_yres)) return;
    if (((x + width) < 0) || ((y + height) < 0)) return;
    //Optimiert:
    int ix;
    int iy;
    int linesize;
    int offset;
    int pagesize;
    int wd_in_bytes;
    if (x + width > screen_xres) width = screen_xres - x;
    if (y + height > screen_yres) height = screen_yres - y;
    int rightedge = x + width;
    int bottomedge = y + height;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    switch (screen_bpp)
    {
        case 8:
            for (iy = y; iy < bottomedge; iy++) {
                memset(backbuffer + x + iy * screen_xres, vgacolor, width);
            }
            //printf("draw_rect(8) 2\n");
            break;
        case 4:
            if (width > 16) {
                pagesize = screen_xres * screen_yres / 8;
                wd_in_bytes = screen_xres / 8;
                for (iy = y; iy < bottomedge; iy++) {
                    int x_in_bytes = (x >> 3);
                    int yoffset = wd_in_bytes * iy;
                    /*for (ix = x; (ix & 7) != 0; ix++) {
                        draw_dot_4p(ix, iy, vgacolor);
                    }*/
                    if (x & 0x7) {
                        byte mask = 0xFF >> (x & 0x7);
                        offset = x_in_bytes + yoffset;
                        int p;
                        int cmask = 1;
                        for (p = 0; p < 4; p++) {
                            byte data = backbuffer[offset];
                            if (vgacolor & cmask) {
                                backbuffer[offset] = data | mask;
                            } else {
                                backbuffer[offset] = data & (~mask);
                            }
                            offset += pagesize;
                            cmask = cmask << 1;
                        }
                        x_in_bytes++;
                    }
                    offset = x_in_bytes + yoffset;
                    linesize = (rightedge >> 3) - x_in_bytes;
                    memset(backbuffer + offset,
                        0xFF * ((vgacolor & 1)), linesize);
                    offset += pagesize;
                    memset(backbuffer + offset,
                        0xFF * ((vgacolor & 2) >> 1), linesize);
                    offset += pagesize;
                    memset(backbuffer + offset,
                        0xFF * ((vgacolor & 4) >> 2), linesize);
                    offset += pagesize;
                    memset(backbuffer + offset,
                        0xFF * ((vgacolor & 8) >> 3), linesize);
                    /*ix = rightedge & 0xFFFFFFF8;
                    for (; ix < rightedge; ix++) {
                        draw_dot_4p(ix, iy, vgacolor);
                    }*/
                    if (rightedge & 0x7) {
                        //ix = rightedge & 0xFFFFFFF8;
                        byte mask = 0xFF << (8 - (rightedge & 0x7));
                        offset = (rightedge >> 3) + yoffset;
                        int p;
                        int cmask = 1;
                        for (p = 0; p < 4; p++) {
                            byte data = backbuffer[offset];
                            if (vgacolor & cmask) {
                                backbuffer[offset] = data | mask;
                            } else {
                                backbuffer[offset] = data & (~mask);
                            }
                            offset += pagesize;
                            cmask = cmask << 1;
                        }
                    }
                }
            } else {
                for (ix = x; ix < rightedge; ix++) {
                    for (iy = y; iy < bottomedge; iy++) {
                        draw_dot_4p(ix, iy, vgacolor);
                    }
                }
            }
            //printf("draw_rect(4) 2\n");
            break;
    }
}
void draw_bitmap(int bitmap, int x, int y, int flags)
{
    //printf("draw_bitmap 1\n");
    if (!backbuffer) {
        return;
    }
    if (!bitmap) {
        return;
    }
    /*int i;
    for (i = 0; i < bitmap_count; i++) {
        if (bitmap_handle[i] == bitmap) {
            //Bitmap gefunden!
            int ix;
            int iy;
            for (ix = 0; ix < bitmap_width[i]; ix++) {
                for (iy = 0; iy < bitmap_height[i]; iy++) {
                        if ((ix + x >= 0) && (ix + x < screen_xres) && (iy + y >= 0) && (iy + y < screen_yres)) {
                            switch (screen_bpp)
                            {
                                case 8:
                                    backbuffer[ix + x + (iy + y) * screen_xres] = 
                                            bitmap_data[i][ix + iy * bitmap_width[i]];
                                    break;
                                case 4:
                                    draw_dot_4p(ix + x, iy + y, bitmap_data[i][ix + iy * bitmap_width[i]]);
                                    break;
                            }
                        }
                }
            }
            return;
        }
    }*/
    //Optimiert:
    int i;
    for (i = 0; i < bitmap_count; i++) {
        if (bitmap_handle[i] == bitmap) {
            //Bitmap gefunden!
            int ix;
            int iy;
            //Zu zeichnenden Bereich berechnen
            int offsetx = 0;
            if (x < 0) offsetx = -x;
            int offsety = 0;
            if (y < 0) offsety = -y;
            int width;
            if ((x + bitmap_width[i]) > screen_xres) {
                width = screen_xres - x - offsetx;
            } else {
                width = bitmap_width[i] - offsetx;
            }
            int height;
            if ((y + bitmap_height[i]) > screen_yres) {
                height = screen_yres - y - offsety;
            } else {
                height = bitmap_height[i] - offsety;
            }
            switch (screen_bpp)
            {
                case 8:
                    for (iy = 0; iy < height; iy++) {
                        memcpy(backbuffer + x + offsetx + (iy + y + offsety) * screen_xres,
                               bitmap_data[i] + offsetx + (iy + offsety) * bitmap_width[i],
                               width);
                    }
                    //printf("draw_bitmap(8) 2\n");
                    return;
                case 4:
                    for (iy = 0; iy < bitmap_height[i]; iy++) {
                        for (ix = 0; ix < bitmap_width[i]; ix++) {
                            if ((ix + x >= 0) && (ix + x < screen_xres) && (iy + y >= 0) && (iy + y < screen_yres)) {
                                draw_dot_4p(ix + x, iy + y, bitmap_data[i][ix + iy * bitmap_width[i]]);
                            }
                        }
                    }
                    //printf("draw_bitmap(4) 2\n");
                    return;
            }
        }
    }
}

int add_bitmap_to_list(int width, int height, byte *data)
{
    //printf("add_bitmap_to_list 1\n");
    byte **tmp_bitmap_data = malloc(sizeof(byte *) * (bitmap_count + 1));
    int *tmp_bitmap_handle = malloc(sizeof(int) * (bitmap_count + 1));
    int *tmp_bitmap_width = malloc(sizeof(int) * (bitmap_count + 1));
    int *tmp_bitmap_height = malloc(sizeof(int) * (bitmap_count + 1));
    memcpy(tmp_bitmap_data, bitmap_data, bitmap_count * sizeof(byte *));
    memcpy(tmp_bitmap_handle, bitmap_handle, bitmap_count * sizeof(int));
    memcpy(tmp_bitmap_width, bitmap_width, bitmap_count * sizeof(int));
    memcpy(tmp_bitmap_height, bitmap_height, bitmap_count * sizeof(int));
    tmp_bitmap_data[bitmap_count] = data;
    tmp_bitmap_width[bitmap_count] = width;
    tmp_bitmap_height[bitmap_count] = height;
    if (bitmap_count > 0) {
        tmp_bitmap_handle[bitmap_count] = tmp_bitmap_handle[bitmap_count - 1] + 1;
        free(bitmap_handle);
        free(bitmap_width);
        free(bitmap_height);
        free(bitmap_data);
    } else {
        tmp_bitmap_handle[bitmap_count] = 1;
    }
    bitmap_handle = tmp_bitmap_handle;
    bitmap_data = tmp_bitmap_data;
    bitmap_width = tmp_bitmap_width;
    bitmap_height = tmp_bitmap_height;
    bitmap_count++;
    //printf("add_bitmap_to_list 2\n");
    return bitmap_handle[bitmap_count - 1];
}
int delete_bitmap_from_list(int index)
{
    //printf("delete_bitmap_from_list 1\n");
    if (index <= 0) {
        return 0;
    }
    if (!bitmap_count) {
        return 0;
    }
    if (index >= bitmap_count) {
        return 0;
    }
    free(bitmap_data[index]);
    if (bitmap_count == 1) {
        free(bitmap_handle);
        free(bitmap_width);
        free(bitmap_height);
        free(bitmap_data);
        bitmap_handle = 0;
        bitmap_data = 0;
        bitmap_width = 0;
        bitmap_height = 0;
        bitmap_count = 0;
    } else {
        byte **tmp_bitmap_data = malloc(sizeof(byte *) * (bitmap_count - 1));
        int *tmp_bitmap_handle = malloc(sizeof(int) * (bitmap_count - 1));
        int *tmp_bitmap_width = malloc(sizeof(int) * (bitmap_count - 1));
        int *tmp_bitmap_height = malloc(sizeof(int) * (bitmap_count - 1));
        memcpy(tmp_bitmap_data, bitmap_data, index * sizeof(byte *));
        memcpy(tmp_bitmap_handle, bitmap_handle, index * sizeof(int));
        memcpy(tmp_bitmap_width, bitmap_width, index * sizeof(int));
        memcpy(tmp_bitmap_height, bitmap_height, index * sizeof(int));
        memcpy(tmp_bitmap_data + index, bitmap_data + index + 1,
            (bitmap_count - index - 1) * sizeof(int *));
        memcpy(tmp_bitmap_handle + index, bitmap_handle + index + 1,
            (bitmap_count - index - 1) * sizeof(int));
        memcpy(tmp_bitmap_width + index, bitmap_width + index + 1,
            (bitmap_count - index - 1) * sizeof(int));
        memcpy(tmp_bitmap_height + index, bitmap_height + index + 1,
            (bitmap_count - index - 1) * sizeof(int));
        free(bitmap_handle);
        free(bitmap_width);
        free(bitmap_height);
        free(bitmap_data);
        bitmap_handle = tmp_bitmap_handle;
        bitmap_data = tmp_bitmap_data;
        bitmap_width = tmp_bitmap_width;
        bitmap_height = tmp_bitmap_height;
        bitmap_count--;
    }
    //printf("delete_bitmap_from_list 2\n");
    return 1;
}

int create_bitmap(int width, int height, int *data)
{
    //printf("create_bitmap 1\n");
    byte *newbitmap = malloc(width * height);
    if (!newbitmap) {
        free(data);
        return 0;
    }
    int i;
    for (i = 0; i < width * height; i++) {
        newbitmap[i] = vga_get_color((dword)data[i]);
    }
    free(data);
    //printf("create_bitmap 2\n");
    return add_bitmap_to_list(width, height, newbitmap);
}
int delete_bitmap(int bitmap)
{
    //printf("delete_bitmap 1\n");
    if (!bitmap) {
        return 0;
    }
    int i;
    for (i = 0; i < bitmap_count; i++) {
        if (bitmap_handle[i] == bitmap) {
            delete_bitmap_from_list(i);
            //printf("delete_bitmap 2\n");
            return 1;
        }
    }
    return 0;
}

int update_screen(void)
{
    //printf("update_screen 1\n");
    if (!backbuffer) { 
        return 0;
    }
    if (!vidmem) {
        return 0;
    }
    //while(inb(0x3DA) & 0x08) yield();
    //while(!(inb(0x3Da)&0x08)) yield();
    //int x, y;
    int pagesize;
    switch (screen_bpp)
    {
        case 8:
            memcpy(vidmem, backbuffer, screen_xres * screen_yres);
            //printf("update_screen(8) 2\n");
            return 1;
        case 4:
            pagesize = screen_xres * screen_yres / 8;
            vga_set_plane(0);
            memcpy(vidmem, backbuffer, pagesize);
            vga_set_plane(1);
            memcpy(vidmem, backbuffer + pagesize, pagesize);
            vga_set_plane(2);
            memcpy(vidmem, backbuffer + pagesize * 2, pagesize);
            vga_set_plane(3);
            memcpy(vidmem, backbuffer + pagesize * 3, pagesize);
            //printf("update_screen(4) 2\n");
            return 1;
    }
    return 0;
}
int update_partly(int x, int y, int width, int height)
{
    if (!backbuffer) {
        return 0;
    }
    if (!vidmem) {
        return 0;
    }
    /*int ix;
    int iy;
    for (ix = 0; ix < width; ix++) {
        for (iy = 0; iy < height; iy++) {
            vga_draw_dot(ix + x, iy + y,
                backbuffer[ix + x + (iy + y) * screen_xres]);
        }
    }
    return 1;*/
    return 0;
}
