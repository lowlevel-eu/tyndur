#ifndef STRING_H
#define STRING_H

#include "types.h"

void * memcpy(void * dest, const void * src, size_t num);
void * memmove(void * dest, const void * src, size_t num);
void * memset(void * addr, int value, size_t len);

#endif /* ndef STRING_H */
