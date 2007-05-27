#ifndef STRING_H
#define STRING_H

#include "types.h"
#include "bsdtypes.h"
#include "ctype.h"

void * memcpy(void * dest, const void * src, size_t num);
void * memmove(void * dest, const void * src, size_t num);
void * memset(void * addr, int value, size_t len);

void * memchr(const void *s, unsigned char c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

char * strcpy(char * dest, const char * src);
char * strncpy(char * dest, const char * src, size_t n);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);

size_t strlen(const char *s);
size_t strnlen(const char *s, size_t maxlen);

char* strstr(const char* s1, const char* s2);
char* strchr(const char* str, int character);
char* strrchr(const char* str, int character);

char* strcat(char* dest, const char* src);

char* strtok(char* str, const char* delim);

void itoa(unsigned int n, char *s, unsigned int base);
unsigned int atoi(const char *s);

char* index(const char *p, int ch);



#endif /* ndef STRING_H */
