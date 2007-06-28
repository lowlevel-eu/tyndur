
#ifndef PALETTE_H
#define PALETTE_H

#include "types.h"

byte vga_get_color_8(dword color);
byte vga_get_color_4(dword color);
byte vga_get_color_2(dword color);
byte vga_get_color_1(dword color);

byte vga_get_color(dword color);

void vga_set_palette_color(int index, byte r, byte g, byte b);
void vga_set_standard_palette(void);

#endif
