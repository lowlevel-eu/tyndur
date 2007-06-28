
#include "palette.h"
#include "vga.h"

#include "ports.h"

byte stdpalette8[256][3] =
{
    //Grautöne
    {0, 0, 0},
    {36, 36, 36},
    {73, 73, 73},
    {109, 109, 109},
    {146, 146, 146},
    {182, 182, 182},
    {219, 219, 219},
    {255, 255, 255},
    //Rot
    {64, 0, 0},
    {128, 0, 0},
    {191, 0, 0},
    {255, 0, 0},
    {64, 32, 32},
    {128, 64, 64},
    {191, 96, 96},
    {255, 128, 128},
    //Gelb
    {64, 64, 0},
    {128, 128, 0},
    {191, 191, 0},
    {255, 255, 0},
    {64, 64, 32},
    {128, 128, 64},
    {191, 191, 96},
    {255, 255, 128},
    //Grün
    {0, 64, 0},
    {0, 128, 0},
    {0, 191, 0},
    {0, 255, 0},
    {32, 64, 32},
    {64, 128, 64},
    {96, 191, 96},
    {128, 255, 128},
    //Cyan
    {0, 64, 64},
    {0, 128, 128},
    {0, 191, 191},
    {0, 255, 255},
    {32, 64, 64},
    {64, 128, 128},
    {96, 191, 191},
    {128, 255, 255},
    //Blau
    {0, 0, 64},
    {0, 0, 128},
    {0, 0, 191},
    {0, 0, 255},
    {32, 32, 64},
    {64, 64, 128},
    {96, 96, 191},
    {128, 128, 255},
    //Violett
    {64, 0, 64},
    {128, 0, 128},
    {191, 0, 191},
    {255, 0, 255},
    {64, 32, 64},
    {128, 64, 128},
    {191, 96, 191},
    {255, 128, 255}
};

byte stdpalette4[16][3] =
{
    {0, 0, 0},
    {0, 0, 255},
    {0, 255, 0},
    {0, 128, 128},
    {255, 0, 0},
    {255, 0, 255},
    {128, 128, 0},
    {192, 192, 192},
    {128, 128, 128},
    {128, 128, 255},
    {128, 255, 128},
    {255, 128, 128},
    {255, 128, 255},
    {128, 255, 255},
    {255, 255, 128},
    {255, 255, 255}
};

byte vga_get_color_8(dword color)
{
    byte bestcolor = 0;

    int diffr = (byte)(((color & 0xFF0000) >> 16) - stdpalette8[0][0]);
    if (diffr < 0) diffr = -diffr;
    int diffg = (byte)(((color & 0xFF00) >> 8) - stdpalette8[0][1]);
    if (diffg < 0) diffg = -diffg;
    int diffb = (byte)((color & 0xFF) - stdpalette8[0][2]);
    if (diffb < 0) diffb = -diffb;
    dword bestdifference = diffr + diffg + diffb;
    int i;
    for (i = 1; i < 56; i++) {
        diffr = (byte)(((color & 0xFF0000) >> 16) - stdpalette8[i][0]);
        if (diffr < 0) diffr = -diffr;
        diffg = (byte)(((color & 0xFF00) >> 8) - stdpalette8[i][1]);
        if (diffg < 0) diffg = -diffg;
        diffb = (byte)((color & 0xFF) - stdpalette8[i][2]);
        if (diffb < 0) diffb = -diffb;
        dword difference = diffr + diffg + diffb;
        if (difference < bestdifference) {
            bestcolor = i;
            bestdifference = difference;
        }
    }
    return bestcolor;
}
byte vga_get_color_4(dword color)
{
    byte bestcolor = 0;

    int diffr = (byte)(((color & 0xFF0000) >> 16) - stdpalette4[0][0]);
    if (diffr < 0) diffr = -diffr;
    int diffg = (byte)(((color & 0xFF00) >> 8) - stdpalette4[0][1]);
    if (diffg < 0) diffg = -diffg;
    int diffb = (byte)((color & 0xFF) - stdpalette4[0][2]);
    if (diffb < 0) diffb = -diffb;
    dword bestdifference = diffr + diffg + diffb;
    int i;
    for (i = 1; i < 16; i++) {
        diffr = (byte)(((color & 0xFF0000) >> 16) - stdpalette4[i][0]);
        if (diffr < 0) diffr = -diffr;
        diffg = (byte)(((color & 0xFF00) >> 8) - stdpalette4[i][1]);
        if (diffg < 0) diffg = -diffg;
        diffb = (byte)((color & 0xFF) - stdpalette4[i][2]);
        if (diffb < 0) diffb = -diffb;
        dword difference = diffr + diffg + diffb;
        if (difference < bestdifference) {
            bestcolor = i;
            bestdifference = difference;
        }
    }
    return bestcolor;
}
byte vga_get_color_2(dword color)
{
    return 0;
}
byte vga_get_color_1(dword color)
{
    return 0;
}

byte vga_get_color(dword color)
{
    switch (screen_bpp)
    {
        case 1:
            return vga_get_color_1(color);
        case 2:
            return vga_get_color_2(color);
        case 4:
            return vga_get_color_4(color);
        case 8:
            return vga_get_color_8(color);
        default:
            return 0;
    }
}

void vga_set_palette_color(int index, byte r, byte g, byte b)
{
    //Einzelne Farbe in den DAC schreiben
    outb(VGA_DAC_WRITE, 0xff); //Alle Farben maskieren (nur eine wird benötigt)
    outb(VGA_DAC_WRITE_INDEX, index);
    outb(VGA_DAC_DATA, r >> 2); //Nur die 6 niedrigen Bits werden beachtet
    outb(VGA_DAC_DATA, g >> 2);
    outb(VGA_DAC_DATA, b >> 2);
}
void vga_set_standard_palette(void)
{
    int i;
    switch (screen_bpp)
    {
        case 1:
            break;
        case 2:
            break;
        case 4:
            for (i = 0; i < 16; i++) {
                vga_set_palette_color(i, stdpalette4[i][0], stdpalette4[i][1],
                                      stdpalette4[i][2]);
            }
            break;
        case 8:
            for (i = 0; i < 256; i++) {
                vga_set_palette_color(i, stdpalette8[i][0], stdpalette8[i][1],
                    stdpalette8[i][2]);
            }
            break;
    }
}
