
#include "types.h"
#include "ports.h"
#include "stdlib.h"
#include "syscall.h"

#include "vbe.h"

int *vidmem = 0;
int *backbuffer = 0;
int screen_xres = 0;
int screen_yres = 0;
int screen_bpp = 0;

void vbe_write(unsigned short index, unsigned short value)
{
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA, value);
}


int vbe_set(unsigned short xres, unsigned short yres, unsigned short bpp)
{
    if (vidmem) {
        mem_free_physical(vidmem, xres * yres * bpp / 8);
        vidmem = 0;
    }
    if (backbuffer) {
        free(backbuffer);
        backbuffer = 0;
    }
    vidmem = mem_allocate_physical(xres * yres * bpp / 8, 0xe0000000, 0);
    if (!vidmem) {
        return 0;
    }
    backbuffer = malloc(xres * yres * bpp / 8);
    if (!backbuffer) {
        mem_free_physical(vidmem, xres * yres * bpp / 8);
        vidmem = 0;
        return 0;
    }
    screen_xres = xres;
    screen_yres = yres;
    screen_bpp = bpp;
    
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    vbe_write(VBE_DISPI_INDEX_XRES, xres);
    vbe_write(VBE_DISPI_INDEX_YRES, yres);
    vbe_write(VBE_DISPI_INDEX_BPP, bpp);
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
    
    return 1;
}

int vbe_init(void)
{
    return vbe_set(800, 600, 32);
}
int vbe_close(void)
{
    if (!vidmem) {
        return 0;
    }
    
    //TODO
    return 0;
}

