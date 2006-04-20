/* Kernel initialisierung erfolgt hier,
 die Funktion main wird von init.asm Aufgerufen */

#include "../include/grub.h"
#include "elf.h"
#include "../include/debug.h"


int init(int eax, multiboot_info_t *boot_info)
{
  char test[4] = "Abc\0";
  print(test,4);
  while(!0) __asm("nop");
  return 0; //Werden wir nie erreichen :-)
}
