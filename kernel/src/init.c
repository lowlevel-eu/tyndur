/* Kernel initialisierung erfolgt hier,
 die Funktion main wird von init.asm Aufgerufen */

#include "../include/grub.h"
#include "elf.h"


int init(int eax, multiboot_info_t *boot_info)
{
  char *test[10];
  *(test) = "Abc";
  print(test);
  while(!0) __asm("nop");
  return 0; //Werden wir nie erreichen :-)
}
