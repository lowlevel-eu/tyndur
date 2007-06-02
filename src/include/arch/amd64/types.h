#ifndef TYPES_H
#define TYPES_H

#define NULL (void*) 0

#ifndef __cplusplus
typedef enum { FALSE = 0, TRUE } bool;
#endif

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;
typedef unsigned long long qword;

typedef signed char sbyte;
typedef signed short sword;
typedef signed int sdword;
typedef signed long long sqword;

typedef __SIZE_TYPE__ size_t;
typedef __SIZE_TYPE__ ssize_t;

typedef void * paddr_t;
typedef void * vaddr_t;

typedef dword pid_t;
typedef dword uid_t;

typedef qword timestamp_t;

typedef qword syscall_id_t;
#endif
