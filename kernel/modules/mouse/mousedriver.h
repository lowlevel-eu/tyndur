/*******************************
  * Maustreiber
  *
  * Aenderungen:
  *
  * 2006-04-11 malu:
  * + Erste Version erstellt.
  ******************************/
#ifndef LOST_MOUSEDRIVER_H
#define LOST_MOUSEDRIVER_H

#define LEFTMOUSEBUTTON   1
#define RIGHTMOUSEBUTTON  2
#define CENTERMOUSEBUTTON 4
#define MOUSEWHEELEUP     8
#define MOUSEWHEELEDOWN  16

bool MouseInit();
int  MouseGetX();
int  MouseGetY();
unsigned char MouseGetPressedButton();
void MouseArea(int x, int y, int w, int h);

#endif // LOST_MOUSEDRIVER_H