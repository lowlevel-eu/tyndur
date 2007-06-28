
#ifndef VGA_MODES_H
#define VGA_MODES_H

//VGA Register
//Text:
extern unsigned char g_40x25_text[];
extern unsigned char g_40x50_text[];
extern unsigned char g_80x25_text[];
extern unsigned char g_80x50_text[];
extern unsigned char g_90x30_text[];
extern unsigned char g_90x60_text[];
//Grafik
extern unsigned char g_640x480x2[];
extern unsigned char g_320x200x4[];
extern unsigned char g_640x480x16[];
extern unsigned char g_720x480x16[];
extern unsigned char g_320x200x256[];
extern unsigned char g_320x200x256_modex[];

#endif
