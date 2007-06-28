
#include "vga.h"
#include "vgamodes.h"
#include "palette.h"

#include "ports.h"
#include "stdlib.h"
#include "syscall.h"

char *vidmem = 0;
char *backbuffer = 0;

int screen_xres = 0;
int screen_yres = 0;
int screen_bpp = 0;
int modex = 0;


dword vga_get_fb_address(void)
{
    dword address;
    outb(VGA_GC_INDEX, 6);
    address = inb(VGA_GC_DATA);
    //Bits 3 und 4 enthalten die Position des Faqmebuffers
    address = (address >> 2) & 3;
    switch(address)
    {
        case 0:
        case 1:
            address = 0xA0000;
            break;
        case 2:
            address = 0xB0000;
            break;
        case 3:
            address = 0xB8000;
            break;
    }
    return address;
}
void vga_write_regs(byte *regs)
{
    dword i;
    //MISC
    outb(VGA_MISC_WRITE, *regs);
    regs++;
    //Sequencer
    for(i = 0; i < VGA_NUM_SEQ_REGS; i++) {
        outb(VGA_SEQ_INDEX, i);
        outb(VGA_SEQ_DATA, *regs);
        regs++;
    }
    //CRTC Register vorbereiten
    outb(VGA_CRTC_INDEX, 0x03);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) | 0x80);
    outb(VGA_CRTC_INDEX, 0x11);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) & ~0x80);
    regs[0x03] |= 0x80;
    regs[0x11] &= ~0x80;
    //CRTC
    for(i = 0; i < VGA_NUM_CRTC_REGS; i++) {
        outb(VGA_CRTC_INDEX, i);
        outb(VGA_CRTC_DATA, *regs);
        regs++;
    }
    //Graphics Controller
    for(i = 0; i < VGA_NUM_GC_REGS; i++) {
        outb(VGA_GC_INDEX, i);
        outb(VGA_GC_DATA, *regs);
        regs++;
    }
    //Attribute Controller
    for(i = 0; i < VGA_NUM_AC_REGS; i++) {
        (void)inb(VGA_INSTAT_READ);
        outb(VGA_AC_INDEX, i);
        outb(VGA_AC_WRITE, *regs);
        regs++;
    }
    //Palette sichern und Modus einschalten
    (void)inb(VGA_INSTAT_READ);
    outb(VGA_AC_INDEX, 0x20);
}
void vga_set_plane(dword plane)
{
    unsigned char pmask = 1 << ( plane & 3);
    //Ebene lesen
    outb(VGA_GC_INDEX, 4);
    outb(VGA_GC_DATA, plane & 3);
    //Ebene auswählen
    outb(VGA_SEQ_INDEX, 2);
    outb(VGA_SEQ_DATA, pmask);
}
void vga_set_pixel_1(dword x, dword y, byte c)
{
}
void vga_set_pixel_2(dword x, dword y, byte c)
{
}
void vga_set_pixel_4p(dword x, dword y, byte c)
{
    unsigned wd_in_bytes, off, mask, p, pmask;

    wd_in_bytes = screen_xres / 8;
    off = wd_in_bytes * y + x / 8;
    x = (x & 7) * 1;
    mask = 0x80 >> x;
    pmask = 1;
    for(p = 0; p < 4; p++)
    {
        vga_set_plane(p);
        if(pmask & c)
            vidmem[off] = vidmem[off] | mask;
        else
            vidmem[off] = vidmem[off] & ~mask;
        pmask <<= 1;
    }
}
void vga_set_pixel_8(dword x, dword y, byte c)
{
    vidmem[x + y * screen_xres] = c;
}
void vga_set_pixel_8x(dword x, dword y, byte c)
{
}

int vga_set(int width, int height, int bpp)
{
    //Aufräumen
    if (vidmem) {
        mem_free_physical(vidmem, width * height);
        vidmem = 0;
    }
    if (backbuffer) {
        free(backbuffer);
        backbuffer = 0;
    }

    //Register des Grafikmodus heraussuchen.
    byte *regs = 0;
    switch (width)
    {
        case 320:
            if (height == 200) {
                switch (bpp)
                {
                    case 2:
                        regs = g_320x200x4;
                        break;
                    case 8:
                        regs = g_320x200x256;
                        break;
                    default:
                        return 0;
                }
            } else {
                return 0;
            }
            break;
        case 640:
            if (height == 480) {
                switch (bpp)
                {
                    case 1:
                        regs = g_640x480x2;
                        break;
                    case 4:
                        regs = g_640x480x16;
                        break;
                    default:
                        return 0;
                }
            } else {
                return 0;
            }
            break;
        case 720:
            if (height == 480) {
                switch (bpp)
                {
                    case 4:
                        regs = g_720x480x16;
                        break;
                    default:
                        return 0;
                }
            } else {
                return 0;
            }
            break;
        default:
            return 0;
    }
    
    //Grafik initialisieren
    vga_write_regs(regs);
    screen_xres = width;
    screen_yres = height;
    screen_bpp = bpp;
    //Speicher allokieren
    vidmem = mem_allocate_physical(width * height, vga_get_fb_address(), 0);
    if (!vidmem) {
        vga_close();
        return 0;
    }
    backbuffer = malloc(width * height);
    if (!backbuffer) {
        vga_close();
        return 0;
    }
    modex = 0;
    return 1;
}
int vga_init(void)
{
    if (!vga_set(640, 480, 4)) return 0;
    //if (!vga_set(320, 200, 8)) return 0;
    vga_set_standard_palette();
    /*int x, y;
    for (x = 0; x < 100; x++) {
        for (y = 0; y < 100; y++) {
            vga_draw_dot(x, y, ((y * 255 / 100) << 16)
                + ((x * y * 255 / 10000) << 8) + x * y * 255 / 10000);
            vga_draw_dot(x + 100, y, ((y * 255 / 100) << 8)
                + ((x * y * 255 / 10000) << 16) + x * y * 255 / 10000);
            vga_draw_dot(x + 200, y, (y * 255 / 100)
                + ((x * y * 255 / 10000) << 16) + ((x * y * 255 / 10000) << 8));
        }
    }*/
    //for(;;);
        /*int i;
        for (i = 0; i < 640 * 480; i++)
        {
            vga_set_pixel_4p(i % 640, i / 640, i % 16);
        }*/
    return 1;
}
int vga_close(void)
{
    if (vidmem) {
        mem_free_physical(vidmem, screen_xres * screen_yres);
        vidmem = 0;
    }
    if (backbuffer) {
        free(backbuffer);
        backbuffer = 0;
    }
    vga_write_regs(g_80x25_text);
    screen_xres = 0;
    screen_yres = 0;
    screen_bpp = 0;
    return 1;
}

void vga_draw_dot(int x, int y, int color)
{
    switch (screen_bpp)
    {
        case 1:
            break;
        case 2:
            break;
        case 4:
            vga_set_pixel_4p(x, y, vga_get_color(color));
            break;
        case 8:
            if (modex) {
                vga_set_pixel_8x(x, y, vga_get_color(color));
            } else {
                vga_set_pixel_8(x, y, vga_get_color(color));
            }
            break;
    }
}
