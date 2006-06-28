#ifndef STRING_H
#define STRING_H

typedef unsigned long size_t; /* wohin gehört das? */

void * memcpy(void * dest, const void * src, size_t num);
void * memset(void *addr, int len, char value);

#endif /* ndef STRING_H */
