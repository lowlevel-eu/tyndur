#ifndef KPRINTF_H
#define KPRINTF_H

/* Gültige Farben. kcolor( (Hintergrund << 4) | Vordergrund);
1 Dunkelblau
2 Dunkelgrün
3 Blaugrün
4 Dunkelrot
5 Violett
6 Ocker
7 Hellgrau
8 Dunkelgrau
9 Blau
A Grün
B Zyan
C Rot
D Magenta
E Gelb
F Weiß
*/

extern void kprintf(char * format, ...);
extern void clrscr();
extern void kcolor(char newcolor);

#endif /* ndef KPRINTF_H */
