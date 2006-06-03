/* Kernel initialisierung erfolgt hier,
 die Funktion main wird von init.asm Aufgerufen */

#include "grub.h"
#include "elf.h"
#include "debug.h"


int init(int eax, multiboot_info_t *boot_info)
{
  char test[4] = "Abc\0";
  print(test,4);

  char output[11] = "__________";
  int2str(boot_info->mem_lower, output);
  print(output, 10);
  
  int2str(boot_info->mem_upper, output);
  print(output, 10);
  
  
  while(!0) __asm("nop");
  return 0; //Werden wir nie erreichen :-)
}
