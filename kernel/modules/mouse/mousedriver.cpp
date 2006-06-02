/*******************************
  * Maustreiber
  *
  * Aenderungen:
  *
  * 2006-04-11 malu:
  * + Erste Version erstellt.
  ******************************/
#include "mousedriver.h"

////////////////////////////////////////
// Function: MouseInit()
// MouseInit() initializes the mouse.
////////////////////////////////////////
bool MouseInit()
{
  bool result = true;
  asm voloatile ("xor %ax, %ax            \n
                 int $0x33                \n
	             cmp $0, %ax              \n
	             je nomouse               \n
	             nomouse:                 \n
				 mov 0, (result)          \n");
  return result;
}

////////////////////////////////////////
// Function: MouseGetX()
// MouseGetX() returns the mouse's
// x-position
////////////////////////////////////////
int MouseGetX()
{
  int x;
  asm volatile ("mov $3, %ax \n
                 int $0x33   \n
				 mov %cx, (x)\n");
  return x;
}

////////////////////////////////////////
// Function: MouseGetY()
// MouseGetY() returns the mouse's
// y-position
////////////////////////////////////////
int MouseGetY()
{
  int y;
  asm volatile ("mov $3, %ax \n
                 int $0x33   \n
				 mov %dx, (y)\n");
  return y;
}

////////////////////////////////////////
// Function: MouseGetPressedButton()
// MouseGetPressedButton() returns the
// pressed button.
// return values:
//   1 = left button
//   2 = right button
//   4 = center button
//   8 = mouse wheele up
//   16 = mouse wheele down
////////////////////////////////////////
unsigned char MouseGetPressedButton()
{
  unsigned char pressed;
  asm volatile ("mov $3, %ax        \n
                 int $0x33          \n
				 mov %bx, (pressed) \n");
  return pressed;
}

////////////////////////////////////////
// Function: MouseArea()
// MouseArea() sets the screen area in
// which the cursor is allowed to be.
////////////////////////////////////////
void MouseArea(int x, int y, int w, int h)
{
  asm volatile ("mov $7, %ax  \n
                 mov (x), %cx \n
				 add (x), (w) \n
				 mov (w), %dx \n
                 int $0x33    \n
				 mov $8, %ax  \n
				 mov (y), %cx \n
				 add (y), (h) \n
				 mov (h), %dx \n
				 int $0x33    \n");
}