#ifndef _STRING_H_
#define _STRING_H_
#include <stddef.h>

void* memcpy(void* dest, const void* src, size_t num);
void* memmove(void* dest, const void* src, size_t num);
void* memset(void* addr, int value, size_t len);

void* memchr(const void* s, unsigned char c, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);

char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);

int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);

size_t strlen(const char* s);
size_t strnlen(const char* s, size_t maxlen);

char* strstr(const char* s1, const char* s2);
char* strchr(const char* str, int character);
char* strrchr(const char* str, int character);

char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, size_t n);

char* strtok(char* str, const char* delim);

size_t strspn(const char* s, const char* charset);
size_t strcspn(const char* s, const char* charset);

void itoa(unsigned int n, char* s, unsigned int base);
unsigned int atoi(const char* s);
long atol(const char* str);

char* index(const char* p, int ch);

char* strdup(const char* str);

char *strpbrk(const char *s1, const char *s2);

char* strerror(int error_code);

int strcasecmp(const char* s1, const char* s2);
int strncasecmp(const char* s1, const char* s2, size_t n);

int strcoll(const char* s1, const char* s2);

#endif /* ndef _STRING_H */

