
#ifndef VGA_H
#define VGA_H

#include "types.h"

extern char *vidmem;
extern char *backbuffer;

extern int screen_xres;
extern int screen_yres;
extern int screen_bpp;

//Ports
//Attribute Controller
#define VGA_AC_INDEX        0x3C0
#define VGA_AC_WRITE        0x3C0
#define VGA_AC_READ         0x3C1
//
#define VGA_MISC_WRITE      0x3C2
//Sequencer
#define VGA_SEQ_INDEX       0x3C4
#define VGA_SEQ_DATA        0x3C5
//Digital-to-Analog-Converter
#define VGA_DAC_WRITE       0x3C6
#define VGA_DAC_READ_INDEX  0x3C7
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA        0x3C9
//
#define VGA_MISC_READ       0x3CC
//Graphics Controller
#define VGA_GC_INDEX        0x3CE
#define VGA_GC_DATA         0x3CF
//CRT Controller
/*                  COLOR emulation     MONO emulation */
#define VGA_CRTC_INDEX      0x3D4       /* 0x3B4 */
#define VGA_CRTC_DATA       0x3D5       /* 0x3B5 */
//Input
#define VGA_INSTAT_READ     0x3DA

#define VGA_NUM_SEQ_REGS    5
#define VGA_NUM_CRTC_REGS   25
#define VGA_NUM_GC_REGS     9
#define VGA_NUM_AC_REGS     21
#define VGA_NUM_REGS        (1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS + \
    VGA_NUM_GC_REGS + VGA_NUM_AC_REGS)


dword vga_get_fb_address(void);
void vga_write_regs(byte *regs);
void vga_set_plane(dword plane);
void vga_set_pixel_1(dword x, dword y, byte c);
void vga_set_pixel_2(dword x, dword y, byte c);
void vga_set_pixel_4p(dword x, dword y, byte c);
void vga_set_pixel_8(dword x, dword y, byte c);
void vga_set_pixel_8x(dword x, dword y, byte c);

int vga_set(int width, int height, int bpp);
int vga_init(void);
int vga_close(void);

void vga_draw_dot(int x, int y, int color);

#endif
