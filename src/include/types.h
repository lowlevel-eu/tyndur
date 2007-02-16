#ifndef TYPES_H
#define TYPES_H

#define NULL (void*) 0

typedef enum { FALSE = 0, TRUE } bool;

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;
typedef unsigned long long qword;

typedef __SIZE_TYPE__ size_t;

typedef void * paddr_t;
typedef void * vaddr_t;

typedef dword pid_t;
typedef dword uid_t;

#endif
