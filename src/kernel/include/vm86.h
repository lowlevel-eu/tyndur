
#ifndef VM86_H
#define VM86_h

#include "tasks.h"

void save_bios_data(void);
word *get_ivt_entry(word interrupt);

struct task * create_vm86_task(word interrupt, vm86_regs_t *regs, dword *meminfo, struct task *parent);

bool vm86_exception(dword *esp);

#endif

