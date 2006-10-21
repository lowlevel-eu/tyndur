#include "syscall.h"
#include "kprintf.h"

void syscall(struct int_stack_frame ** esp) {
  struct int_stack_frame * isf = *esp;

  // FIXME: Adressen, die vom aufrufenden Programm kommen, 
  // vor der Verwendung prüfen
  switch (isf->eax) {
    case 0:
      kprintf("%s", *((char**) isf->esp));
      break;
  }
}
