#include "gdt.h"
#include "tss.h"

tss_t tss = { .ss0 = SYS_DATA_SEL };


