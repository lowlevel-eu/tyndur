#ifndef STRING_H
#define STRING_H

#include "types.h"
#include "bsdtypes.h"

void * memcpy(void * dest, const void * src, size_t num);
void * memmove(void * dest, const void * src, size_t num);
void * memset(void * addr, int value, size_t len);

char * strcpy(char * dest, const char * src);
char * strncpy(char * dest, const char * src, size_t n);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

size_t strlen(const char *s);
size_t strnlen(const char *s, size_t maxlen);

char* strstr(const char* s1, const char* s2);

void itoa(unsigned int n, char *s, unsigned int base);
unsigned int atoi(const char *s);
#endif /* ndef STRING_H */
