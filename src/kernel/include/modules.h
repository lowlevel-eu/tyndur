#ifndef MODULES_H
#define MODULES_H

#include "multiboot.h"

void load_init_module(struct multiboot_info * multiboot_info);
void load_multiboot_modules(struct multiboot_info * multiboot_info);

#endif
